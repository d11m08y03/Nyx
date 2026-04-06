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
#include "chapters/lit_review.typ"
#include "chapters/analysis.typ"
#include "chapters/design.typ"
#include "chapters/implementation.typ"
#include "chapters/testing.typ"
#include "chapters/conclusion.typ"

#pagebreak()
#bibliography("references.bib", style: "harvard-cite-them-right")
