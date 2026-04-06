= Introduction <introduction>

== Background

Modern astronomy is mostly a data science field. The amount of data from space missions has been growing at an exponential rate in the last two decades. Missions like the Kepler mission and its successor the Transiting Exoplanet Survey Satellite (TESS) have contributed to this growth. The Kepler mission, which launched in 2009, observed 150,000 stars for four years and led to the discovery of the first large number of exoplanets @borucki2010. The TESS mission, which launched in 2018, has observed 26 different areas of the sky for approximately 27 days each @ricker2015. TESS collects 27 GB of data per orbit, enabling high-cadence brightness measurements of hundreds of thousands of stars per orbit. All of the data collected by TESS is stored in NASA's Mikulski Archive for Space Telescopes (MAST). MAST stores data from numerous astronomical missions, with over 500 TB of stored data as of 2024 @stsdci2024.

The main technique that will be employed in both of these missions is the technique known as transit photometry, in which the dimming of a star’s brightness due to the presence of a planet in front of the star is detected. The data that is obtained from this method is a light curve, which plots the brightness of the star over time. The depth of the dips in the light curve indicates the radius of the planet, and the period of the dips indicates the planet’s orbital period @winn2010. Furthermore, light curves can also be used to study other astronomical phenomena beyond planetary transits. Thus, the ability to analyse light curves is an essential skill within the field of astronomy.

Simultaneously, ground-based amateur astronomy has become increasingly capable of performing photometry of variable stars with the availability of affordable telescope equipment and software @fitzgerald2014. Even relatively inexpensive telescopes, such as a 130mm Newtonian reflector mounted on an equatorial mount, and even using smartphones as the cameras, it is possible to perform imaging of certain variable stars @gary2007. Furthermore, organisations like the American Association of Variable Star Observers (AAVSO) and the British Astronomical Association (BAA) utilise the observations of amateurs to contribute to the understanding of variable stars and exoplanets @conti2018. The observations of ground-based telescopes can help to supplement those of space-based telescopes through the ability to provide longer-term observations of variable stars, to observe periods between space-based telescope observation windows, and through the use of ground-based telescopes to confirm the observations of space-based telescopes.

However, a disconnect exists between these two data sources. The nature of the TESS observations is completely different to ground-based observations. The TESS observations are kept in FITS files, a binary file format standard in astronomy that contains 2-minute or 30-minute calibrated flux measurements @tenenbaum2018. These files use the Barycentric TESS Julian Date (BTJD) time system to account for the movement of the TESS satellite compared to the Solar System barycentre @eastman2010. In contrast, amateur ground-based observations are stored as JPEG or FITS files in which the flux must be extracted from the images, and the measurement of time uses either the UTC or Julian Date time systems. Therefore, to integrate these two data sources, the software will need to parse binary FITS files, account for differences in the time systems, deal with different noise profiles in the data, and normalize the flux measurements from each type of observation, all of which presents a data engineering challenge.

Mauritius, located at 20.3° S latitude and 57.5° E longitude in the Indian Ocean, possesses a number of geographic advantages for ground-based astronomical observations. First, its location in the southern hemisphere provides visibility of numerous astronomical objects that are inaccessible from most observatories in the northern hemisphere. Objects such as the Magellanic Clouds, the Galactic Centre, and a plethora of exoplanet hosting stars are visible from the southern hemisphere but not visible from the northern @ricker2015. 
Furthermore, during TESS’ first year of operation (July 2018 -- July 2019), its primary observation target was the southern portion of the ecliptic plane, after which it performed observations of the northern portion of the ecliptic plane @ricker2015. Second, Mauritius lies in a longitudinal gap in relation to most major observatory locations in the southern hemisphere. The majority of ground-based observatories in the southern hemisphere are located in Chile (\~70° W), South Africa (\~18° E) and eastern Australia (\~150° E) @hessman2004. 
Thus, an observer on Mauritius can collect observational data during the night time when none of these three locations are observing the skies. Despite these geographic advantages for performing astronomical observations, there is currently no infrastructure for astronomical observation on the island, and no existing platform for comparing in-situ observations with data collected by spacecraft like TESS.

== Problem Statement

High-quality phonometric data is available from NASA’s open data archives. The archives are, however, aimed primarily at professional researchers. The MAST API that NASA employs returns data in JSON format, which requires some knowledge of the domain to effectively use. To construct a query to the API, one must manually construct a JSON request that includes the relevant parameters and filters for the observation that is to be queried. The returned data is in the form of a JSON object whose column names, types, and values are returned as separate arrays @stsdci2024. To answer a relatively simple question of interest to amateur astronomers, such as “how does my ground-based observation of a given star compare to that made by the TESS satellite?” an observer must manually query the MAST API for the relevant data, download the observations in the FITS file format, parse the file with libraries such as CFITSIO @pence2010, and plot the results with Python scripts or other software such as Lightkurve @lightkurve2018. Each one of these steps indicates a level of expertise in Python and in the TESS and MAST APIs that acts as a barrier of entry for amateur astronomers.

Some tools are available that aim to make the process of querying the TESS satellite data and comparing it to ground-based observations easier. Lightkurve, for instance, provides a Python API that allows researchers to download and analyse light curves from the Kepler and TESS satellites; however, as a programmatic library rather than a web application it does not allow for integration with ground-based observations @lightkurve2018. The Exoplanet Transit Database allows amateurs to submit their observations of planetary transits, but provides no access to TESS data @poddany2010. The AAVSO International Database also allows amateurs to submit their light curve observations, but mainly of variable stars rather than exoplanet candidates @kafka2021.

There is currently no platform that integrates the following functions:

- Resolving astronomical objects by name
- Retrieval of TESS data
- Parsing of ground-based and TESS data
- Storage of data in a relational database
- API for frontend integration
- Platform for amateurs to upload their observations

This problem is not an astrophysics problem but a data engineering problem. The data exists; the problem is to build the system that appropriately ingests, transforms, stores, and retrieves the data.

== Aim and Objectives

The aim of this project is to design and implement a data-driven astronomical observation platform that integrates NASA space mission data with ground-based observations captured from Mauritius.

The objectives are:

+ Design and implement a backend using modern C++ that exposes a RESTful API for synchronous operations, background job processing for long-running data ingestion tasks, and Server-Sent Events (SSE) for real-time progress updates to the frontend. The architecture should follow clean architecture principles with clear separation between domain logic, data access, and presentation layers.
+ Build a data ingestion pipeline that queries NASA's MAST API, retrieves TESS observation metadata, discovers data products, and downloads and parses FITS light curve files into structured time-series data.
+ Implement a PostgreSQL-backed persistence layer that stores astronomical targets, TESS observations, and light curve data points in a schema optimised for time-series queries.
+ Develop an authentication and authorisation system supporting local registration with email verification and third-party OAuth2, with security measures including JWT token rotation, CSRF protection, and rate limiting.
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
