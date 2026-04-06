#import "template.typ": project
#show: body => project(title: "Test", subtitle: "Test", author: "Test", student-id: "0", supervisor: "Test", degree: "Test", university: "Test", date: "2026", body)
#include "chapters/design.typ"
#bibliography("references.bib", style: "harvard-cite-them-right")
