#heading(numbering: none)[Glossary]

#let term(name, description) = [
  / #text(weight: "bold", name): #description
]

#term("API")[Application Programming Interface. A set of defined rules enabling software components to communicate.]

#term("BJD")[Barycentric Julian Date. A time standard that corrects for the observer's position relative to the Solar System barycentre, removing orbital motion effects from timing measurements.]

#term("BTJD")[Barycentric TESS Julian Date. The time system used by TESS, corrected for light travel time to the Solar System barycentre. Defined as BJD − 2457000.0.]

#term("CAOM")[Common Archive Observation Model. A standardised data model used by MAST to describe astronomical observations across multiple missions.]

#term("CSRF")[Cross-Site Request Forgery. An attack where a malicious site submits requests on behalf of an authenticated user. Mitigated using the double-submit cookie pattern.]

#term("ETL")[Extract, Transform, Load. A data pipeline pattern for ingesting data from external sources, transforming it into a target format, and loading it into a database.]

#term("FFI")[Full-Frame Image. A TESS data product capturing the entire field of view of a camera at 30-minute (primary mission) or 10-minute (extended mission) cadence.]

#term("FITS")[Flexible Image Transport System. The standard file format in astronomy for storing images, spectra, and tabular data. Defined by the IAU FITS Working Group.]

#term("Flux")[The amount of light received from an astronomical object per unit area per unit time, typically measured in electrons per second (e⁻/s) for space-based photometry.]

#term("HDU")[Header Data Unit. The fundamental building block of a FITS file, consisting of an ASCII header followed by an optional data array.]

#term("JWT")[JSON Web Token. A compact, URL-safe token format used for stateless authentication. Contains encoded claims signed with a secret key.]

#term("Light Curve")[A time-series of flux measurements for an astronomical object, used to detect transits, eclipses, and variability.]

#term("MAST")[Mikulski Archive for Space Telescopes. NASA's primary archive for ultraviolet, optical, and near-infrared astronomical data, hosted by the Space Telescope Science Institute.]

#term("OAuth2")[Open Authorization 2.0. An industry-standard protocol for delegated authentication, allowing users to sign in via third-party providers such as Google.]

#term("PDCSAP Flux")[Pre-search Data Conditioning Simple Aperture Photometry flux. TESS flux values corrected for systematic errors such as spacecraft pointing jitter and thermal variations.]

#term("Photometry")[The measurement of the intensity of light from astronomical objects. Differential photometry measures brightness changes relative to comparison stars in the same field.]

#term("RAII")[Resource Acquisition Is Initialisation. A C++ idiom where resource lifetime is tied to object lifetime, ensuring deterministic cleanup via destructors.]

#term("REST")[Representational State Transfer. An architectural style for web APIs using standard HTTP methods (GET, POST, PUT, DELETE) with stateless request-response semantics.]

#term("SAP Flux")[Simple Aperture Photometry flux. Raw flux values extracted from TESS pixel data by summing counts within a defined aperture, before systematic correction.]

#term("SPOC")[Science Processing Operations Center. The NASA Ames facility responsible for processing raw TESS data into calibrated light curves and target pixel files.]

#term("SSE")[Server-Sent Events. A unidirectional HTTP-based protocol for pushing real-time updates from server to client over a persistent connection.]

#term("SSR")[Server-Side Rendering. A web rendering strategy where HTML is generated on the server rather than in the browser, improving initial load performance.]

#term("TESS")[Transiting Exoplanet Survey Satellite. A NASA space telescope launched in 2018 that surveys nearly the entire sky for exoplanet transits using transit photometry.]

#term("TOI")[TESS Object of Interest. A star identified by TESS as hosting a candidate transiting planet, pending confirmation by follow-up observations.]

#term("TPF")[Target Pixel File. A TESS data product containing the raw pixel data for a small region around a target star, used for custom aperture photometry.]

#term("Transit")[The passage of a planet in front of its host star as seen from the observer, causing a measurable dip in the star's brightness.]
