pub trait ParsePath {
    fn parse_path(&self) -> String;
}

impl ParsePath for String {
    fn parse_path(&self) -> String {
        if self.starts_with("~/") {
            return dirs::home_dir().unwrap().join(self.strip_prefix("~/").unwrap()).to_string_lossy().parse::<String>().unwrap();
        } else {
            self.clone()
        }
    }
}
