#import "template.typ": project

#show: body => project(
  title: "Nyx",
  subtitle: "A Data-Driven Astronomical Observation Platform Using NASA Space Data and Local Observations in Mauritius",
  author: "Zakariyya Kurmally",
  student-id: "2315839",
  supervisor: "Mr Somveer Kisna",
  degree: "BSc (Hons) Data Science",
  university: "University of Mauritius",
  date: "2026",
  body,
)

#include "frontmatter/abstract.typ"
#pagebreak()
#outline(indent: auto)
#pagebreak()
#include "frontmatter/glossary.typ"
#pagebreak()

#include "chapters/introduction.typ"

#pagebreak()
#include "chapters/lit_review.typ"

#pagebreak()
#include "chapters/analysis.typ"

#pagebreak()
#include "chapters/design.typ"

#pagebreak()
#include "chapters/implementation.typ"

#pagebreak()
#include "chapters/testing.typ"

#pagebreak()
#include "chapters/conclusion.typ"

#pagebreak()
#bibliography("references.bib", style: "harvard-cite-them-right")
