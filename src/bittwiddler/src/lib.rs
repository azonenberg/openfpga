extern crate proc_macro;
extern crate syn;
#[macro_use]
extern crate quote;

use proc_macro::TokenStream;
use std::collections::HashSet;

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

fn parse_multi_string_attr_helper<T, F>(attrs: &[syn::Attribute], attr_name: &str, cb: F)
    -> std::iter::Map<std::vec::IntoIter<syn::LitStr>, F>
    where F: FnMut(syn::LitStr) -> T {

    let mut ret = Vec::new();
    for attr in attrs {
        if attr.path.leading_colon.is_none() && attr.path.segments.len() == 1 &&
           attr.path.segments[0].ident.as_ref() == attr_name {
            let attr_val = attr.interpret_meta();
            if attr_val.is_none() {
                panic!("Failed to parse {} attribute", attr_name);
            }

            if let syn::Meta::NameValue(nameval) = attr_val.unwrap() {
                if let syn::Lit::Str(litstr) = nameval.lit {
                    ret.push(litstr);
                } else {
                    panic!("{} attribute must be set to a string", attr_name)
                }
            } else {
                panic!("Malformed {} attribute", attr_name);
            }
        }
    }

    ret.into_iter().map(cb)
}

#[derive(Copy, Clone, Debug)]
enum BitTwiddlerFieldRef {
    Ident(syn::Ident),
    Index(usize),
    Self_,
}

impl BitTwiddlerFieldRef {
    fn to_string(&self) -> String {
        match self {
            &BitTwiddlerFieldRef::Ident(id) => id.as_ref().to_owned(),
            &BitTwiddlerFieldRef::Index(idx) => format!("{}", idx),
            &BitTwiddlerFieldRef::Self_ => "Enum".to_owned(),
        }
    }
}

#[derive(Debug)]
struct StringSplitter<'a> {
    str: &'a str,
    striter: std::str::CharIndices<'a>,
    // is_in_quote: bool,
    ended: bool,
    ungetbuf: Option<(usize, char)>,
}

impl<'a> StringSplitter<'a> {
    fn new(s: &'a str) -> Self {
        StringSplitter {
            str: s,
            striter: s.char_indices(),
            ended: false,
            ungetbuf: None,
        }
    }

    fn getchar(&mut self) -> Option<(usize, char)> {
        if self.ungetbuf.is_some() {
            let ret = self.ungetbuf;
            self.ungetbuf = None;
            ret
        } else {
            self.striter.next()
        }
    }

    fn ungetchar(&mut self, c: (usize, char)) {
        assert!(self.ungetbuf.is_none());
        self.ungetbuf = Some(c);
    }
}

impl<'a> Iterator for StringSplitter<'a> {
    type Item = &'a str;
    fn next(&mut self) -> Option<&'a str> {
        if self.ended {
            None
        } else {
            // Eat leading spaces
            let mut c = self.getchar();
            while c.is_some() && c.unwrap().1 == ' ' {
                c = self.getchar();
            }
            if c.is_none() {
                self.ended = true;
                return None;
            }
            // self.ungetchar(c.unwrap());

            // At this point we have at least 1 character and c is pointing to it
            let start_of_ret_idx = c.unwrap().0;
            let mut end_of_ret_idx = start_of_ret_idx;
            let mut is_in_quote = false;
            while c.is_some() {
                end_of_ret_idx = c.unwrap().0;
                if !is_in_quote {
                    if c.unwrap().1 == ' ' {
                        self.ungetchar(c.unwrap());
                        break;
                    }

                    if c.unwrap().1 == '\'' {
                        is_in_quote = true;
                    }

                    c = self.getchar();
                } else {
                    if c.unwrap().1 == '\'' {
                        is_in_quote = false;
                    }

                    c = self.getchar();
                }
            }
            // We have reached the end while returning the current item
            if c.is_none() {
                end_of_ret_idx = self.str.len();
            }

            unsafe { Some(self.str.slice_unchecked(start_of_ret_idx, end_of_ret_idx)) }
        }
    }
}

fn is_bool(ty: &syn::Type) -> bool {
    if let &syn::Type::Path(ref typep) = ty {
        if typep.qself.is_none() && typep.path.leading_colon.is_none() && typep.path.segments.len() == 1 &&
            typep.path.segments[0].ident.as_ref() == "bool" {

            true
        } else {
            false
        }
    } else {
        false
    }
}

#[proc_macro_derive(BitTwiddler, attributes(bittwiddler, bittwiddler_field))]
pub fn bittwiddler(input: TokenStream) -> TokenStream {
    let input: syn::DeriveInput = syn::parse(input).unwrap();

    let test1 = StringSplitter::new("asdf   fdsa   'ab c d e f'g hij");
    let test2 = test1.collect::<Vec<_>>();
    println!("{:?}", test2);

    let test1 = StringSplitter::new("asdf   fdsa   'ab c d e f'g hij ");
    let test2 = test1.collect::<Vec<_>>();
    println!("{:?}", test2);

    let main_bittwiddler_attribs = parse_multi_string_attr_helper(&input.attrs, "bittwiddler",
        |x| x.value()).collect::<Vec<_>>();
    println!("{:?}", main_bittwiddler_attribs);

    let mut fields_and_attrs = Vec::new();

    match input.data {
        syn::Data::Struct(datastruct) => {
            // let input_ident = input.ident;
            // println!("{:?}", input_ident);

            match datastruct.fields {
                syn::Fields::Named(named) => {
                    for field in &named.named {
                        let id = field.ident.unwrap();
                        let bittwiddler_field_attrs = parse_multi_string_attr_helper(&field.attrs, "bittwiddler_field",
                            |x| x.value()).collect::<Vec<_>>();

                        // println!("{:?} {:?}", id, bittwiddler_field_attrs);
                        // println!("{:?}", field.ty);
                        let is_bool = is_bool(&field.ty);

                        if bittwiddler_field_attrs.len() > 0 {
                            fields_and_attrs.push((BitTwiddlerFieldRef::Ident(id), bittwiddler_field_attrs, is_bool));
                        }
                    }
                },
                syn::Fields::Unnamed(unnamed) => {
                    for (idx, field) in unnamed.unnamed.iter().enumerate() {
                        let bittwiddler_field_attrs = parse_multi_string_attr_helper(&field.attrs, "bittwiddler_field",
                            |x| x.value()).collect::<Vec<_>>();

                        // println!("{:?} {:?}", idx, bittwiddler_field_attrs);
                        let is_bool = is_bool(&field.ty);

                        if bittwiddler_field_attrs.len() > 0 {
                            fields_and_attrs.push((BitTwiddlerFieldRef::Index(idx), bittwiddler_field_attrs, is_bool));
                        }
                    }
                },
                syn::Fields::Unit => {},
            }
        },
        syn::Data::Enum(_dataenum) => {
            // let input_ident = input.ident;
            // println!("{:?}", input_ident);
            // unimplemented!();

            let bittwiddler_field_attrs = parse_multi_string_attr_helper(&input.attrs, "bittwiddler_field",
                |x| x.value()).collect::<Vec<_>>();

            if bittwiddler_field_attrs.len() > 0 {
                fields_and_attrs.push((BitTwiddlerFieldRef::Self_, bittwiddler_field_attrs, false));
            }
        },
        _ => panic!("BitPattern must be used on a struct or enum")
    }

    let input_ident = input.ident;
    println!("{:?}", input_ident);
    println!("{:?}", fields_and_attrs);

    let mut all_tokens = quote!{};

    for bittwiddler_instance_attrib in main_bittwiddler_attribs {
        let mut attrib_split = StringSplitter::new(&bittwiddler_instance_attrib);
        let instance_name = attrib_split.next().unwrap();

        let instance_attribs = attrib_split.collect::<Vec<_>>();
        println!("{:?}", instance_attribs);
        let mut instance_attribs_hash = HashSet::with_capacity(instance_attribs.len());
        for x in instance_attribs {
            instance_attribs_hash.insert(x);
        }

        let this_instance_attribs = fields_and_attrs.iter().map(|x|
            (x.0, x.1.iter().map(|y| StringSplitter::new(y).collect::<Vec<_>>()).filter(|z|
                z[0] == instance_name).collect::<Vec<_>>(), x.2));

        println!("{}", instance_name);
        // println!("{:?}", this_instance_attribs.collect::<Vec<_>>());

        // TODO: Look for the error attribute

        let mut dimensions = None;

        let encode_fn_ident = syn::Ident::from(format!("encode_{}", instance_name));

        let mut encode_field_tokens = quote!{};

        for (field_id, field_locs, field_isbool) in this_instance_attribs {
            if field_locs.len() == 0 {
                continue;
            }

            // println!("{:?} {:?} {:?}", field_id, field_locs, field_isbool);

            if field_locs.len() > 1 {
                panic!("{} has multiple bittwiddler_field for type {}", field_id.to_string(), instance_name);
            }
            let mut field_locs = &field_locs[0][1..];

            let mut needs_err = false;
            if field_locs[0] == "err" {
                needs_err = true;
                field_locs = &field_locs[1..];
            }

            println!("{:?} {:?} {:?} {:?}", field_id, field_locs, field_isbool, needs_err);

            if field_isbool && field_locs.len() != 1 {
                panic!("Field {} of {} on {} has too many locations for a boolean",
                    field_id.to_string(), instance_name, input_ident.as_ref());
            }

            // Now we can actually generate code
            let mut encode_this_field = if !field_isbool {
                match field_id {
                    BitTwiddlerFieldRef::Ident(id) => {
                        if needs_err {
                            quote! {
                                let x = self.#id.encode()?;
                            }
                        } else {
                            quote! {
                                let x = self.#id.encode();
                            }
                        }
                    },
                    BitTwiddlerFieldRef::Index(idx) => {
                        if needs_err {
                            quote! {
                                let x = self.#idx.encode()?;
                            }
                        } else {
                            quote! {
                                let x = self.#idx.encode();
                            }
                        }
                    },
                    BitTwiddlerFieldRef::Self_ => {
                        if needs_err {
                            quote! {
                                let x = self.encode()?;
                            }
                        } else {
                            quote! {
                                let x = self.encode();
                            }
                        }
                    }
                }
            } else {
                if needs_err {
                    panic!("Field {} of {} on {} has \"err\" flag, but it's a boolean",
                        field_id.to_string(), instance_name, input_ident.as_ref());
                }
                match field_id {
                    BitTwiddlerFieldRef::Ident(id) => {
                        quote! {
                            let x = self.#id;
                        }
                    },
                    BitTwiddlerFieldRef::Index(idx) => {
                        quote! {
                            let x = self.#idx;
                        }
                    },
                    BitTwiddlerFieldRef::Self_ => {
                        unreachable!();
                    }
                }
            };

            for (field_bit_i, loc) in field_locs.iter().enumerate() {
                let mut loc = *loc;
                let inv = loc.get(0..1) == Some("!");
                if inv {
                    loc = loc.split_at(1).1;
                }

                let coords = loc.split('|').collect::<Vec<_>>();

                // println!("{:?} {:?}", inv, coords);

                if dimensions.is_none() {
                    dimensions = Some(coords.len());
                } else {
                    if dimensions.unwrap() != coords.len() {
                        panic!("Instance {} on {} has mismatched dimensions", instance_name, input_ident.as_ref());
                    }
                }

                let mut index_each_dim = Vec::with_capacity(coords.len());
                for (dim_i, coord_str) in coords.iter().enumerate() {
                    let mirror_attrib = format!("mirror{}", dim_i);
                    let mirror = instance_attribs_hash.contains::<str>(&mirror_attrib);
                    let coord_i = coord_str.parse::<usize>().expect("Could not parse coordinate as number");

                    let dim_idx = syn::Index {
                        index: dim_i as u32,
                        // TODO: IDK wtf to do here?!
                        span: input_ident.span()
                    };

                    if mirror {
                        let mirror_ident = syn::Ident::from(format!("mirror_{}", dim_i));
                        // mirror_idents.push(quote!{#mirror_ident: bool});
                        index_each_dim.push(quote! {
                            (start_coord.#dim_idx as isize + (if #mirror_ident {-1} else {1}) * #coord_i as isize) as usize
                        });
                    } else {
                        index_each_dim.push(quote! {
                            start_coord.#dim_idx + #coord_i
                        });
                    }
                }

                // println!("{:?}", index_each_dim);

                let field_bit_idx = syn::Index {
                    index: field_bit_i as u32,
                    // TODO: IDK wtf to do here?!
                    span: input_ident.span()
                };
                if !field_isbool {
                    encode_this_field.append_all(quote! {
                        fuses[(#(#index_each_dim),*)] = x.#field_bit_idx;
                    });
                } else {
                    encode_this_field.append_all(quote! {
                        fuses[(#(#index_each_dim),*)] = x;
                    });
                }
            }

            // println!("{:?}", encode_this_field);
            encode_field_tokens.append_all(encode_this_field);
        }

        if dimensions.is_none() {
            panic!("Instance {} on {} has zero fields", instance_name, input_ident.as_ref());
        }

        let mut usize_idents = Vec::with_capacity(dimensions.unwrap());
        for _ in 0..dimensions.unwrap() {
            usize_idents.push(quote!{usize});
        }
        let mut usize_idents2 = Vec::with_capacity(dimensions.unwrap());
        for _ in 0..dimensions.unwrap() {
            usize_idents2.push(quote!{usize});
        }
        let mut usize_idents3 = Vec::with_capacity(dimensions.unwrap());
        for _ in 0..dimensions.unwrap() {
            usize_idents3.push(quote!{usize});
        }

        let mut mirror_idents = Vec::new();
        for dim_i in 0..dimensions.unwrap() {
            let mirror_attrib = format!("mirror{}", dim_i);
            let mirror = instance_attribs_hash.contains::<str>(&mirror_attrib);

            if mirror {
                let mirror_ident = syn::Ident::from(format!("mirror_{}", dim_i));
                mirror_idents.push(quote!{#mirror_ident: bool});
            }
        }

        let ispub_token = if instance_attribs_hash.contains("pub") {
            quote!{pub}
        } else {
            quote!{}
        };

        let mut encode_tokens = quote!{
            impl #input_ident {
                #ispub_token fn #encode_fn_ident<T>(&self,
                    mut fuses: T, start_coord: (#(#usize_idents),*), #(#mirror_idents),*) 

                    where T: ::std::ops::IndexMut<(#(#usize_idents2),*), Output=bool>,
                    // <T as ::std::ops::Index<(#(#usize_idents3),*)>>::Output: bool
                {
                    #encode_field_tokens
                }
            }
        };

        // encode_tokens.append_all(quote!{}});
        println!("{:?}", encode_tokens);
        all_tokens.append_all(encode_tokens);
    }

    all_tokens.into()
}
