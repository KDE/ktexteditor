To install this highlighting file run:

```
# Clone
git clone https://github.com/rust-lang/kate-config.git`

# The following differs between Kate 4.x and 5.x, see below:

# For Kate 4.x:
mkdir --parents ~/.kde/share/apps/katepart/syntax
cp kate-config/rust.xml ~/.kde/share/apps/katepart/syntax

# For Kate 5.x:
mkdir --parents ~/.local/share/apps/katepart5/syntax
cp kate-config/rust.xml ~/.local/share/apps/katepart5/syntax
```

Then from Kate, open a Rust file. If Rust highlighting
isn't enabled, enable this setting in the menus:

Tools -> Highlighting -> Sources -> Rust

## License

Rust is primarily distributed under the terms of both the MIT license
and the Apache License (Version 2.0), with portions covered by various
BSD-like licenses.

See [LICENSE-APACHE](LICENSE-APACHE), [LICENSE-MIT](LICENSE-MIT), and [COPYRIGHT](COPYRIGHT) for details.
