extern crate proc_macro;
extern crate syn;
#[macro_use]
extern crate quote;

use proc_macro::TokenStream;

#[proc_macro_derive(BitPattern, attributes(bits))]
pub fn bitpattern(input: TokenStream) -> TokenStream {

    // println!("{:#?}", input.to_string());

    // Parse the input tokens into a syntax tree
    let input: syn::DeriveInput = syn::parse(input).unwrap();

    if let (input_ident, syn::Data::Enum(dataenum)) = (input.ident, input.data) {
        // println!("asdf");

        if dataenum.variants.len() == 0 {
            return (quote!{}).into();
        }

        let mut var_bits = Vec::new();

        for var in &dataenum.variants {
            // println!("{}", var.ident);

            if var.fields != syn::Fields::Unit {
                panic!("All variants must be a unit variant");
            }

            let mut found_bits_attr = false;
            for attr in &var.attrs {
                if attr.path.leading_colon.is_none() && attr.path.segments.len() == 1 &&
                   attr.path.segments[0].ident.as_ref() == "bits" {

                    if found_bits_attr {
                        panic!("Only one bits attribute allowed");
                    }

                    found_bits_attr = true;
                    // println!("{:?}", attr.path);

                    let bits_val = attr.interpret_meta().expect("Failed to parse bits attribute");

                    if let syn::Meta::NameValue(nameval) = bits_val {
                        if let syn::Lit::Str(litstr) = nameval.lit {
                            // println!("{:#?}", litstr.value());
                            var_bits.push((var.ident, litstr.value()));
                        } else {
                            panic!("bits attribute must be set to a string")
                        }
                    } else {
                        panic!("Malformed bits attribute");
                    }
                }
            }

            if !found_bits_attr {
                panic!("All variants need a bits attribute");
            }
        }

        let bits_len = var_bits[0].1.len();
        for x in &var_bits[1..] {
            if x.1.len() != bits_len {
                panic!("All bits need to be the same length");
            }
        }

        for x in &var_bits {
            for c in x.1.chars() {
                if c != '0'  && c != '1' && c != 'x' && c != 'X' {
                    panic!("Illegal character in bits attribute");
                }
            }
        }

        // println!("{:#?}", var_bits);

        // Literally a list of bool tokens, repeated <number of bits> times
        let bools = var_bits[0].1.chars().map(|_| quote! {bool});
        let bools2 = var_bits[0].1.chars().map(|_| quote! {bool});
        // The name of the enum, repeated <number of variants> times
        let idents_dummy_list = var_bits.iter().map(|_| input_ident);
        let idents_dummy_list2 = var_bits.iter().map(|_| input_ident);
        // The names of each variant
        let var_names = var_bits.iter().map(|x| x.0);
        let var_names2 = var_bits.iter().map(|x| x.0);

        // The list of values for encoding
        let encode_values = var_bits.iter().map(|x|
            x.1.chars().map(|c|
                match c {
                    '0'|'x' => quote! {false},
                    '1'|'X' => quote! {true},
                    _ => unreachable!(),
                }
            )
        );

        // The list of values for decoding
        let decode_values = var_bits.iter().map(|x|
            x.1.chars().map(|c|
                match c {
                    '0' => quote! {false},
                    '1' => quote! {true},
                    'x'|'X' => quote! {_},
                    _ => unreachable!(),
                }
            )
        );

        (quote! {
            impl #input_ident {
                pub fn encode(&self) -> (#(#bools),*) {
                    match *self {
                        #(#idents_dummy_list::#var_names => (#(#encode_values),*)),*
                    }
                }

                pub fn decode(x: (#(#bools2),*)) -> Self {
                    match x {
                        #((#(#decode_values),*) => #idents_dummy_list2::#var_names2),*
                    }
                }
            }
        }).into()
    } else {
        panic!("BitPattern must be used on an enum");
    }
}
