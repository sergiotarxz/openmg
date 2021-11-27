const char *const MIGRATIONS[] = {
    ("CREATE TABLE options (\n"
    "key TEXT PRIMARY KEY,\n"
    "value TEXT\n"
    ");\n"),
    ("CREATE TABLE image (\n"
    "url TEXT PRIMARY KEY,\n"
    "file TEXT\n"
    ");\n"),
};
