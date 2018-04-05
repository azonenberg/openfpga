extern crate proc_macro;
extern crate syn;
#[macro_use]
extern crate quote;

use proc_macro::TokenStream;

fn parse_string_attr_helper<T, F>(attrs: &[syn::Attribute], attr_name: &str, cb: F) -> Option<T>
    where F: FnOnce(syn::LitStr) -> T {

    let mut ret = None;
    for attr in attrs {
        if attr.path.leading_colon.is_none() && attr.path.segments.len() == 1 &&
           attr.path.segments[0].ident.as_ref() == attr_name {
            if ret.is_some() {
                panic!("Only one {} allowed", attr_name);
            }

            let attr_val = attr.interpret_meta();
            if attr_val.is_none() {
                panic!("Failed to parse {} attribute", attr_name);
            }

            if let syn::Meta::NameValue(nameval) = attr_val.unwrap() {
                if let syn::Lit::Str(litstr) = nameval.lit {
                    ret = Some(litstr);
                } else {
                    panic!("{} attribute must be set to a string", attr_name)
                }
            } else {
                panic!("Malformed {} attribute", attr_name);
            }
        }
    }

    ret.map(cb)
}

#[proc_macro_derive(BitPattern, attributes(bits, bits_default, bits_errtype))]
pub fn bitpattern(input: TokenStream) -> TokenStream {
    // Parse the input tokens into a syntax tree
    let input: syn::DeriveInput = syn::parse(input).unwrap();

    let bits_default: Option<syn::Expr> = parse_string_attr_helper(&input.attrs, "bits_default",
        |x| x.parse().expect("Failed to parse bits_default attribute contents"));

    let bits_errtype: Option<syn::TypeParam> = parse_string_attr_helper(&input.attrs, "bits_errtype",
        |x| x.parse().expect("Failed to parse bits_errtype attribute contents"));

    if let (input_ident, syn::Data::Enum(dataenum)) = (input.ident, input.data) {
        // Ignore enums with no variants
        if dataenum.variants.len() == 0 {
            println!("Warning: BitPattern used on enum with no variants");
            return (quote!{}).into();
        }

        let mut var_bits = Vec::new();

        for var in &dataenum.variants {
            if var.fields != syn::Fields::Unit {
                panic!("All variants must be a unit variant");
            }

            let bits_attr = parse_string_attr_helper(&var.attrs, "bits", |x| x)
                .expect("All variants need a bits attribute");
            var_bits.push((var.ident, bits_attr.value()));
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

        let mut encode_tokens = quote! {
            impl #input_ident {
                pub fn encode(&self) -> (#(#bools),*) {
                    match *self {
                        #(#idents_dummy_list::#var_names => (#(#encode_values),*)),*
                    }
                }
            }
        };

        let deocde_tokens = if bits_errtype.is_none() {
            quote! {
                impl #input_ident {
                    pub fn decode(x: (#(#bools2),*)) -> Self {
                        match x {
                            #((#(#decode_values),*) => #idents_dummy_list2::#var_names2),*
                            #(,_ => #bits_default)*
                        }
                    }
                }
            }
        } else {
            let bits_errtype = bits_errtype.unwrap();
            quote! {
                impl #input_ident {
                    pub fn decode(x: (#(#bools2),*)) -> Result<Self, #bits_errtype> {
                        Ok(match x {
                            #((#(#decode_values),*) => #idents_dummy_list2::#var_names2),*
                            #(,_ => return Err(#bits_default))*
                        })
                    }
                }
            }
        };

        encode_tokens.append_all(deocde_tokens);
        encode_tokens.into()
    } else {
        panic!("BitPattern must be used on an enum");
    }
}
