#heading(numbering: none)[Glossary] <glossary>

#let term(name, key, description) = [
  / #text(weight: "bold", name) #label(key): #description
]

#term("API", "gl:api")[Application Programming Interface. A set of defined rules enabling software components to communicate.]

#term("BTJD", "gl:btjd")[Barycentric TESS Julian Date. The time system used by TESS, corrected for light travel time to the Solar System barycentre. Defined as BJD − 2457000.0.]

#term("CSRF", "gl:csrf")[Cross-Site Request Forgery. An attack where a malicious site submits requests on behalf of an authenticated user. Mitigated using the double-submit cookie pattern.]

#term("ETL", "gl:etl")[Extract, Transform, Load. A data pipeline pattern for ingesting data from external sources, transforming it into a target format, and loading it into a database.]

#term("FITS", "gl:fits")[Flexible Image Transport System. The standard file format in astronomy for storing images, spectra, and tabular data. Defined by the IAU FITS Working Group.]

#term("Flux", "gl:flux")[The amount of light received from an astronomical object per unit area per unit time, typically measured in electrons per second (e⁻/s) for space-based photometry.]

#term("JWT", "gl:jwt")[JSON Web Token. A compact, URL-safe token format used for stateless authentication. Contains encoded claims signed with a secret key.]

#term("Light Curve", "gl:light-curve")[A time-series of flux measurements for an astronomical object, used to detect transits, eclipses, and variability.]

#term("MAST", "gl:mast")[Mikulski Archive for Space Telescopes. NASA's primary archive for ultraviolet, optical, and near-infrared astronomical data, hosted by the Space Telescope Science Institute.]

#term("OAuth2", "gl:oauth2")[Open Authorization 2.0. An industry-standard protocol for delegated authentication, allowing users to sign in via third-party providers such as Google.]

#term("PDCSAP Flux", "gl:pdcsap")[Pre-search Data Conditioning Simple Aperture Photometry flux. TESS flux values corrected for systematic errors such as spacecraft pointing jitter and thermal variations.]

#term("Photometry", "gl:photometry")[The measurement of the intensity of light from astronomical objects. Differential photometry measures brightness changes relative to comparison stars in the same field.]

#term("REST", "gl:rest")[Representational State Transfer. An architectural style for web APIs using standard HTTP methods (GET, POST, PUT, DELETE) with stateless request-response semantics.]

#term("SAP Flux", "gl:sap")[Simple Aperture Photometry flux. Raw flux values extracted from TESS pixel data by summing counts within a defined aperture, before systematic correction.]

#term("SSE", "gl:sse")[Server-Sent Events. A unidirectional HTTP-based protocol for pushing real-time updates from server to client over a persistent connection.]

#term("TESS", "gl:tess")[Transiting Exoplanet Survey Satellite. A NASA space telescope launched in 2018 that surveys nearly the entire sky for exoplanet transits using transit photometry.]

#term("Transit", "gl:transit")[The passage of a planet in front of its host star as seen from the observer, causing a measurable dip in the star's brightness.]
