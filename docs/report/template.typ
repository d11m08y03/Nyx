#let project(
  title: "",
  subtitle: "",
  author: "",
  student-id: "",
  supervisor: "",
  degree: "",
  university: "",
  date: "",
  body,
) = {
  // Page Setup
  set page(
    paper: "a4",
    margin: (top: 2.5cm, bottom: 2.5cm, left: 3cm, right: 2.5cm),
    numbering: "1",
    number-align: center,
  )

  // Text Settings
  set text(
    font: "Times New Roman",
    size: 12pt,
    lang: "en"
  )

  // Paragraph Settings: Justified + 1.5 line spacing
  set par(
    justify: true,
    leading: 0.65em,
    first-line-indent: 0em,
  )

  // Space between paragraphs
  set par(spacing: 1.5em)

  // Heading Settings
  set heading(numbering: "1.1.")
  show heading: it => {
    set block(above: 2em, below: 1em)
    it
  }

  // Title Page
  page(numbering: none)[
    #set align(center)
    #v(2fr)

    #text(size: 24pt, weight: "bold", title)

    #v(0.5em)

    #text(size: 14pt, subtitle)

    #v(2em)

    #text(size: 14pt, author)

    #text(size: 12pt, "Student ID: " + student-id)

    #v(1em)

    #text(size: 12pt, degree)

    #text(size: 12pt, university)

    #v(1em)

    #text(size: 12pt, "Supervisor: " + supervisor)

    #v(1em)

    #text(size: 12pt, date)

    #v(2fr)
  ]

  body
}
