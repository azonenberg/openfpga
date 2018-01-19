use std::io::Write;

fn one_set_of_reftests(outfile: &'static str, indir: &'static str) {
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let destination = std::path::Path::new(&out_dir).join(outfile);
    let mut f = std::fs::File::create(&destination).unwrap();

    let root_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let reftests_dir = std::path::Path::new(&root_dir).join(indir);
    let files = std::fs::read_dir(reftests_dir).unwrap();

    for file in files {
        let path = file.expect("failed to get path").path();
        let ext = path.extension().expect("bogus reftest filename (doesn't have extension)");
        if ext == "json" || ext == "fail" {
            let path = path.canonicalize().unwrap();

            let id_string = path.file_name().unwrap().to_str().unwrap().chars().map(|x| match x {
                'A'...'Z' | 'a'...'z' | '0'...'9' => x,
                _ => '_',
            }).collect::<String>();

            if ext == "fail" {
                write!(f, "#[should_panic]\n").unwrap();
            }
            write!(f, r#"
                #[test]
                fn reftest_{}() {{
                    run_one_reftest("{}");
                }}
                "#, id_string, path.to_str().unwrap()).unwrap();
        }
    }
}

fn main() {
    one_set_of_reftests("frontend-reftests.rs", "../../tests/xc2par/frontend-reftests");
    one_set_of_reftests("netlist-reftests.rs", "../../tests/xc2par/netlist-reftests");
}
