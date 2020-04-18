extern crate data_encoding;
extern crate md5;

use std::{env, fs};
use std::time::{SystemTime};
use std::collections::HashMap;

use md5::{Context, Digest};
use std::fs::File;
use std::io::{BufReader, Read};


#[derive(Debug)]
struct FileList {
    src: Vec<FileData>,
    dest: Vec<FileData>
}

#[derive(Debug, Clone)]
struct FileData {
    name: String,
    file_path: String,
    size: u64,
    hash: String
}

fn md5_digest(path: String) -> Result<Digest, std::io::Error> {
    
    let input = File::open(path)?;
    let mut reader = BufReader::new(input);
    
    let mut context = Context::new();
    let mut buffer = [0; 1024];

    loop {
        let count = reader.read(&mut buffer)?;
        if count == 0 {
            break;
        }
        context.consume(&buffer[..count]);
    }

    Ok(context.compute())
}

fn walk_files(path: String, files: &mut Box<HashMap<String, FileData>>) -> Result<(), std::io::Error> {

    for entry in fs::read_dir(path).unwrap() {
        let entry = entry.expect("failed getting file in dir");
        let entry_path = entry.path();

        let metadata = fs::metadata(&entry_path).expect("failed getting metadata");

        if metadata.is_file() {
            let base_name = entry_path.file_name().unwrap().to_str().expect("Failed getting base name");
            let file_size = metadata.len();
            let last_modified = metadata.modified()?.duration_since(SystemTime::UNIX_EPOCH).expect("Failed getting modified date");

            let file_path = entry_path.to_str().expect("failed unwrapping string").to_string();

            let file_data : FileData = FileData {
                name: base_name.to_string(),
                file_path: file_path.clone(),
                size: file_size,
                hash: String::default(),
            };

            //let digest = md5_digest(entry_path.to_str().unwrap().to_string());
            
            files.insert(format!("{}-{}", file_data.name, file_data.size), file_data);

            //println!("{:<40} {:>15} {:>15} {}", base_name, file_size, last_modified.as_secs(), &file_path.clone());
        } else if metadata.is_dir() {
            //println!("{}", entry_path.to_str().unwrap().to_string());
            let _ = walk_files(entry_path.to_str().unwrap().to_string(), files);
        }
    }

    return Ok(());
}

fn main() -> () {
    let args: Vec<String> = env::args().collect();


    let mut path : String = env::current_dir().unwrap().to_str().unwrap().to_lowercase();
    if args.len() > 2 {
        path = args[0].to_string();
    }

    let mut files : Box<HashMap<String, FileData>> = Box::new(HashMap::new());

    println!("{}", path);
    walk_files(path, &mut files).unwrap();

    for (key, value) in files.iter() {
        println!("{:<40} {:<40} {:>15} {}", key, value.name, value.size, &value.file_path.clone());    
    }
    
}
