#import "../frontmatter/utils.typ": gls

= Introduction

== Background

Modern astronomy is fundamentally a data science discipline. The volume of observational data produced by space-based missions has grown exponentially over the past two decades, driven by missions such as Kepler and its successor, the #gls("Transiting Exoplanet Survey Satellite (TESS)", key: "gl:tess"). Kepler, launched in 2009, monitored approximately 150,000 stars in a single field of view for four years, producing the first large-scale statistical census of exoplanets @borucki2010. TESS, launched in 2018, expanded this approach to a near-all-sky survey, dividing the celestial sphere into 26 observation sectors and monitoring each for approximately 27 days @ricker2015. TESS generates approximately 27 GB of data per orbit, producing high-cadence photometric time-series --- brightness measurements over time --- for hundreds of thousands of stars. These datasets are publicly available through NASA's #gls("Mikulski Archive for Space Telescopes (MAST)", key: "gl:mast"), which hosts over 500 TB of astronomical data as of 2024 @stsdci2024.

The primary technique underlying both missions is #gls("transit", key: "gl:transit") #gls("photometry", key: "gl:photometry"): detecting the periodic dimming of a star's brightness as a planet passes in front of it. The resulting data product is a #gls("light curve", key: "gl:light-curve") --- a time-series of #gls("flux", key: "gl:flux") (brightness) measurements. The depth of a transit dip reveals the planet's radius relative to the star, while the period between dips gives the orbital period @winn2010. Light curves are also used to study stellar variability, eclipsing binary systems, and other astrophysical phenomena. The ability to analyse these time-series datasets is central to modern observational astronomy.

Simultaneously, ground-based amateur astronomy has become increasingly capable. Affordable reflecting telescopes, motorised equatorial mounts, and open-source data reduction software have enabled amateur astronomers to produce scientifically useful photometric measurements @fitzgerald2014. Even entry-level instruments such as a 130mm Newtonian reflector on an equatorial mount, paired with a smartphone camera or a dedicated CMOS sensor, can capture images suitable for differential photometry of bright variable stars and exoplanet transits @gary2007. Organisations such as the American Association of Variable Star Observers (AAVSO) and the British Astronomical Association (BAA) routinely incorporate amateur observations into professional research programmes, particularly for variable star monitoring and exoplanet transit confirmation @conti2018. Ground-based observations complement space-based data in several ways: they can provide longer temporal baselines, fill gaps between space mission observation windows, and offer independent confirmation of detected signals.

However, a disconnect exists between these two data sources. Space-based and ground-based observations differ in cadence, precision, noise characteristics, and data formats. TESS observations are stored in #gls("FITS (Flexible Image Transport System)", key: "gl:fits") files --- a binary format standard in astronomy --- containing calibrated flux measurements at 2-minute or 30-minute intervals @tenenbaum2018. These files use the #gls("Barycentric TESS Julian Date (BTJD)", key: "gl:btjd") time system, which corrects for the satellite's orbital motion relative to the Solar System barycentre @eastman2010. Ground-based amateur observations are typically captured as JPEG or FITS images requiring photometric extraction before analysis, and use UTC or Julian Date time systems. Integrating these heterogeneous sources into a unified pipeline requires parsing different binary formats, reconciling time systems, handling different noise profiles (systematic spacecraft trends vs. atmospheric scintillation), and normalising flux scales --- a non-trivial data engineering challenge.

Mauritius, located at 20.3\u{b0}S latitude and 57.5\u{b0}E longitude in the Indian Ocean, offers several geographic advantages for ground-based astronomy. First, its southern hemisphere position provides visibility of targets that are less accessible from major Northern Hemisphere observatories, including the Magellanic Clouds, the Galactic Centre, and southern-sky exoplanet hosts. This is particularly relevant to TESS, which devoted its first year of operations (July 2018 -- July 2019) to the southern ecliptic hemisphere before surveying the north @ricker2015, meaning the densest TESS coverage aligns with the sky visible from Mauritius. Second, Mauritius occupies a longitude gap between major professional observatory sites. The bulk of southern hemisphere observing capacity is concentrated in Chile (~70\u{b0}W), South Africa (~18\u{b0}E), and eastern Australia (~150\u{b0}E) @hessman2004. An observer at 57.5\u{b0}E can capture data during nighttime hours when none of these three sites are observing, providing temporal coverage that would otherwise be missing from ground-based follow-up campaigns. Despite these advantages, there is limited astronomical observation infrastructure on the island, and no existing platform that enables local observers to systematically compare their observations against space-based data.

== Problem Statement

NASA's open data archives provide high-quality photometric time-series data, but they are designed primarily for professional researchers. The MAST #gls("API", key: "gl:api") uses a columnar JSON response format that requires domain-specific knowledge to query and interpret. A single MAST query involves constructing a JSON request specifying service name, parameters, and data filters, then parsing a response where column names, types, and values are returned as separate arrays rather than as conventional row-oriented records @stsdci2024. For an observer who wants to answer a simple question --- "how does my ground-based measurement of star X compare to what TESS observed?" --- the current workflow involves manually querying MAST, downloading FITS files, parsing binary table extensions using libraries such as CFITSIO @pence2010, converting between BTJD and Julian Date time systems, and plotting the results using scripts or specialised tools such as Lightkurve @lightkurve2018. This multi-step process requires proficiency in Python, familiarity with astronomical data formats, and knowledge of the MAST API --- a significant barrier for amateur observers.

Several existing tools partially address this problem. Lightkurve provides a Python API for downloading and analysing Kepler and TESS light curves, but it is a programmatic library, not a web platform, and offers no mechanism for integrating user observations @lightkurve2018. The Exoplanet Transit Database (ETD) allows amateurs to submit transit observations, but does not provide direct access to TESS data or automated comparison @poddany2010. The AAVSO International Database accepts photometric submissions but is focused on variable star monitoring rather than exoplanet transit analysis, and does not offer integrated visualisation of space-based reference data @kafka2021.

No existing platform provides an integrated pipeline that:
- Resolves astronomical targets by name against authoritative catalogues.
- Retrieves and caches TESS observation metadata and light curve data.
- Parses FITS binary tables into structured, queryable time-series stored in a relational database.
- Exposes this data through a #gls("REST", key: "gl:rest") API suitable for frontend visualisation.
- Allows users to upload their own ground-based observations and overlay them against space-based data for comparison.

This gap is not primarily an astrophysics problem --- it is a data engineering problem. The scientific data already exists. The challenge lies in building a performant, well-architected pipeline that ingests, transforms, stores, and serves heterogeneous astronomical data from multiple sources.

== Aim and Objectives

The aim of this project is to design and implement a data-driven astronomical observation platform that integrates NASA space mission data with ground-based observations captured from Mauritius.

The objectives are:

+ Design and implement a backend using modern C++ that exposes a RESTful API for synchronous operations, background job processing for long-running data ingestion tasks, and #gls("Server-Sent Events (SSE)", key: "gl:sse") for real-time progress updates to the frontend. The architecture should follow clean architecture principles with clear separation between domain logic, data access, and presentation layers.
+ Build a data ingestion pipeline that queries NASA's MAST API, retrieves TESS observation metadata, discovers data products, and downloads and parses FITS light curve files into structured time-series data.
+ Implement a PostgreSQL-backed persistence layer that stores astronomical targets, TESS observations, and light curve data points in a schema optimised for time-series queries.
+ Develop an authentication and authorisation system supporting local registration with email verification and third-party #gls("OAuth2", key: "gl:oauth2"), with security measures including #gls("JWT", key: "gl:jwt") token rotation, #gls("CSRF", key: "gl:csrf") protection, and rate limiting.
+ Create an observation management module that allows authenticated users to record ground-based observation sessions with associated equipment metadata and captured images.
+ Build a frontend application that provides interactive light curve visualisation, a target catalogue browser, and data overlay capabilities for comparing space-based and ground-based observations.
+ Validate the system by ingesting real TESS data for known exoplanet host stars and comparing the resulting light curves against published values.

== Scope and Limitations

This project covers the full-stack development of the Nyx platform, from database schema design through API implementation to frontend visualisation. The primary data source is NASA's TESS mission, accessed via the MAST API. The system is designed to be extensible to other NASA data sources such as the Exoplanet Archive, but integration with additional archives is not a primary deliverable.

The following are explicitly out of scope:

- *Real-time telescope control.* The platform manages observation data after capture, not during acquisition.
- *Automated plate-solving or astrometry.* Users specify their target by name; the system does not attempt to identify targets from image content.
- *Advanced photometric reduction.* The system accepts pre-reduced photometric measurements from ground-based observations. Full aperture photometry from raw images is not implemented.
- *Multi-site deployment.* The system is designed for single-instance deployment. Distributed architecture and horizontal scaling are not addressed.

The default observation site is Mauritius (longitude 57.5\u{b0}E, latitude 20.3\u{b0}S), which is used for visibility calculations and observation planning features.

== Report Structure

The remainder of this report is organised as follows. @lit_review reviews existing astronomical data platforms, data pipeline architectures, and relevant technologies, establishing the academic and technical context for the project. @analysis presents the requirements analysis, including functional and non-functional requirements derived from the problem statement and stakeholder needs. @design describes the system architecture, database schema design, API design, and key design decisions with their rationale. @implementation details the technical implementation of each major subsystem, including the data ingestion pipeline, authentication system, and frontend application. @testing covers the testing strategy, including unit tests, integration tests, and validation against known astronomical data. @conclusion evaluates the project against its objectives, discusses lessons learned, and identifies future work.
