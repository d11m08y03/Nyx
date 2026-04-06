// Link to a glossary entry: #gls("TESS", key: "gl:tess")
// The 'display' argument is the text shown; 'key' is the label in the glossary.
#let gls(display, key: none) = {
  let target = if key != none { key } else { "gl:" + lower(display) }
  link(label(target), display)
}
