use std::path::PathBuf;

fn main() {
    let headers = std::fs::read_dir("../src/include/rust")
        .expect("failed to read headers");

    for entry in headers {
        let path = entry.unwrap().path();

        if path.extension().and_then(|s| s.to_str()) == Some("h") {
            let filename = path.file_stem().unwrap().to_str().unwrap();

            let bindings = bindgen::Builder::default()
                .header(path.to_str().unwrap())
                .generate()
                .expect("Unable to generate bindings");

            let out_path = PathBuf::from("ffi");
            std::fs::create_dir_all(&out_path).unwrap();

            bindings
                .write_to_file(out_path.join(format!("{}.rs", filename)))
                .expect("Couldn't write bindings!");
        }
    }
}
