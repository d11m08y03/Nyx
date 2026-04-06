= Literature Review <lit_review>

In this chapter, we review the existing academic and technical context for the tool that we are to develop. Topics covered in this chapter include the sources and formats of astronomical data, the existing platforms and tools in this area, data pipeline architecture, software architecture, database architecture for time-series data, authentication and security, API design, and the frontend technologies for scientific data visualisation.

== Astronomical Data Sources

=== The TESS Mission

The Transiting Exoplanet Survey Satellite (TESS) is a NASA Astrophysics Explorer mission that aims to detect exoplanets in most of the sky @ricker2015. Unlike its predecessor the Kepler mission that observed a small patch of the sky for four years @borucki2010, TESS observes 26 different sectors of the sky, each measuring 24\u{b0} × 96\u{b0} in area. Each of the four instruments on the TESS spacecraft can observe 24\u{b0} × 96\u{b0} areas of the sky, allowing TESS to survey 2300 square degrees of the sky with its full suite of instruments @ricker2015.

The mission produces two main types of data products from these observations. Full-Frame Images (FFIs) and Target Pixel Files (TPFs) and Light Curve Files are collected by the mission and available for researchers. FFIs and TPFs are collected at a cadence of 30 minutes (though reduced to 10 minutes during the extended mission of TESS). The TPFs and Light Curve Files are collected at 2-minute cadence for pre-selected targets by the Science Processing Operations Center (SPOC) at NASA Ames @jenkins2016. This Science Processing Operations Center processes the raw data from TESS to produce two different flux measurements for each target TESS observes: the Simple Aperture Photometry (SAP) flux (the flux of the target within an optimal aperture) and the Pre-search Data Conditioning SAP (PDCSAP) flux (an enhanced flux measurement that applies corrections for systematics of the TESS instruments, such as the pointing of the TESS instruments @stumpe2012).

All of the data products from TESS are stored in the FITS file format and made available through the Mikulski Archive for Space Telescopes (MAST). TESS has completed its primary mission (sectors 1-26) and its first extended mission (sectors 27-55). TESS is currently observing during its second extended mission, having observed over 85% of the sky with its survey and discovering more than 7,000 TESS Objects of Interest (TOIs) @guerrero2021.

=== The FITS Data Format

The Flexible Image Transport System (FITS) @pence2010 is the data format standard in astronomy, defined by the International Astronomical Union (IAU) FITS Working Group. A FITS file contains one or more Header Data Units (HDUs). Each HDU contains an ASCII header that describes the file contents, and the data itself, which can be in either ASCII or binary format. The primary HDU (designation “primary”) contains the image data or no data at all; additional HDUs are used to contain different data like binary tables.

The light curves of the TESS mission are stored in the binary table extensions of the FITS file. The data contained in these tables include columns such as TIME, SAP_FLUX, PDCSAP_FLUX, PDCSAP_FLUX_ERR, and QUALITY @tenenbaum2018. These tables can be parsed using a FITS library, such as CFITSIO @pence2010, or a general binary file parser that can recognize the FITS file specification. The QUALITY column of these tables is critical to understanding the light curves; any non-zero values in this column indicate measurements of the light curve that should be disregarded in any analysis of the light curve.

The BTJD time system used by the mission is the Barycentric Julian Date minus 2,457,000.0 days. The Barycentric Julian Date is the actual time measurements from the ground observers (in UTC or Julian Date) corrected for the length of time that it takes for light to travel from the object in space to the observer on the ground; this value is at most 8.3 minutes (the light travel time across 1 AU) @eastman2010. Thus, while the ground observers use one time standard, the TESS mission uses a different time standard, which can be converted to one another through the use of the geographic and celestial coordinates of the observer and target object.

=== The MAST Archive

The Mikulski Archive for Space Telescopes (MAST) is the main storage facility for data from space telescopes that observe the universe in the ultraviolet, visible, and near-infrared parts of the electromagnetic spectrum. MAST is hosted by the Space Telescope Science Institute (STScI) in Baltimore @stsdci2024. MAST provides an API to access their stored data. Data can be accessed by making an HTTP POST request to the following URL: https://mast.stsci.edu/api/v0/invoke. The API accepts a JSON object with the service that is to be used and the parameters and filters for that service.

The data that is returned by the API is in a “columnar” format; the JSON response includes three separate arrays: one for the names of the columns of the data, one for the type of each of those columns, and one for the values of each row of the data @stsdci2024. The relevant services for accessing TESS data are:

- Mast.Name.Lookup: uses the name of an object in space (such as a star) to return its location in the sky
- Mast.Caom.Filtered.Position: searches for observations of an area of the sky
- Mast.Caom.Products: retrieves the data products from a given sky observation

== Existing Platforms and Tools

=== Lightkurve

Lightkurve is an open-source Python package for analysing time-series data from the Kepler and TESS missions @lightkurve2018. Lightkurve provides a high-level API for accessing data directly from MAST, performing operations like light curve flattening, folding and periodogram analysis. The package uses the module to access the archives, and the library for reading FITS files.

While Lightkurve does lower the barrier to analyzing data from the TESS mission for Python developers, it is a library, not a web application. It is dependent upon having a local installation of Python, the use of Jupyter notebooks, and does not have any functionality for managing observations or comparing them to the available TESS data. Any data is not persisted by the software, and must be downloaded each session.

=== Exoplanet Transit Database

The Exoplanet Transit Database is a website created and operated by the Czech Astronomical Society that allows amateur astronomers to upload their observations of the transits of exoplanets @poddany2010. Each observation can be fitted to a model to create a catalogue of Transit Timing Variations (TTVs) for the planets included in the database.

The ETD does not include functions to allow users to view TESS light curves or to ingest data from MAST. Furthermore, the database is focused upon the timing of the transits of the planets rather than providing a means for the comparison of light curves to those of known exoplanets.

=== AAVSO International Database

The American Association of Variable Star Observers (AAVSO) maintains one of the largest databases of measurements of the brightness of variable stars, receiving over 43 million star brightness measurements from observers around the world @kafka2021. The AAVSO collects data on variable stars and makes it available through various online databases and tools.

The AAVSO performs measurements of the brightness of variable stars over time. It does not perform surveys for exoplanets or provide a way of cross-referencing its observations with those from space-based astronomy missions. It maintains a database of brightness measurements of variable stars, but does not provide visualisation of brightness measurements from space-based observatories.

=== ExoFOP

The Exoplanet Follow-up Observing Program (ExoFOP) is a web-based platform that is operated by the NASA Exoplanet Science Institute (NExScI) and dedicated to the TESS Objects of Interest @exofop2019. The website collects observations of these candidates that have been performed by the astronomical community.

ExoFOP is dedicated to the professional astronomical follow-up community. It does not have functionality for light curve visualisation, uploading light curves from amateur astronomers, or managing observations. Thus, it serves a different audience than a platform dedicated to amateur astronomers comparing their light curves to those from TESS.

=== Gap Analysis

The platform descriptions above cover a subset of the overall problem. Table @gap_analysis_table provides a summary of the capabilities of each of these platforms for five key features of the platform we wish to create.

#figure(
  table(
    columns: (auto, auto, auto, auto, auto),
    align: (left, center, center, center, center),
    stroke: 0.5pt,
    inset: 6pt,
    table.header(
      [*Platform*], [*TESS Data Access*], [*User Observations*], [*Light Curve Visualisation*], [*Web-Based*],
    ),
    [Lightkurve], [Yes], [No], [Yes (Python)], [No],
    [ETD], [No], [Yes], [Limited], [Yes],
    [AAVSO], [No], [Yes], [No], [Yes],
    [ExoFOP], [Links only], [Professional only], [No], [Yes],
  ),
  caption: [Comparison of existing platforms across key features.],
) <gap_analysis_table>

No existing platform combines direct TESS data ingestion, amateur observation management, and interactive web-based light curve visualisation with data overlay capabilities.

== Data Pipeline Architecture

=== ETL and ELT Patterns

Data pipelines for ingesting external data typically use either Extract, Transform, Load (ETL) or Extract, Load, Transform (ELT) patterns @kimball2013. In ETL patterns, data is extracted from the external system, transformed into the target system schema, and then loaded into the target system. In ELT patterns, the data is loaded into the target system in its raw form, and transformed within the target system using SQL or stored procedures.

ETL patterns are typically used when the transformations are complex and best performed with a general-purpose programming language. ELT patterns are typically used when the transformations can be expressed in SQL, and when the data volumes are large enough that loading the data first is efficient @kimball2013.

Idempotency requires that the same input to a system produce the same output upon repeated invocations. For scientific data systems, idempotency can be achieved by using natural keys from the observation system to create a unique constraint on the data @kleppmann2017.

=== Batch, Streaming, and On-Demand Processing

Data ingestion can be implemented in three different ways: batch, streaming, and on-demand @kleppmann2017.

Given the infrequent update cycles of the data sources, such as MAST which releases new TESS sectors approximately every 27 days, it is appropriate to use batch or on-demand data ingestion strategies. Streaming data ingestion strategies will add unnecessary infrastructure to the system. An on-demand data ingestion strategy, in which data is imported only when it is requested by a user, represents a compromise between avoiding bulk importing of data that may never be used, and providing fast data access to users through caching @kleppmann2017.

=== Server-Sent Events for Progress Reporting

Many of the tasks we wish to perform on the server, such as downloading and parsing FITS files, will take several seconds to complete. Server-Sent Events (SSE) is a W3C standard that allows for a server to push data to a client over a single HTTP connection @w3c_sse2015. Unlike WebSockets, SSE connections are one-way (from server to client only), utilize standard HTTP protocols, and are automatically supported by the browser’s EventSource API. Thus, SSE connections are suitable for situations like reporting the progress of a server task to a client browser.

== Software Architecture

=== Clean Architecture

Proposed by Martin (2017), the architecture consists of layers of software components whose dependencies must point inward to the inner layers of the software @martin2017. The innermost layer contains the business rules that define the enterprise’s business domain, followed by layers that contain application business rules, interface adapters, and finally the frameworks and drivers that implement the software’s functionality.

Each of these layers can be tested independently of the rest of the software. For example, the domain and application business rules can be tested using unit tests that mock the dependencies on the interface adapters, database, and other external services. The dependencies of each layer point to abstractions rather than specific implementations of those components and services, which is referred to as the dependency inversion principle @martin2000. In the context of C++, these abstractions are represented as pure virtual classes, with their implementations injected through the constructors of the classes.

=== Hexagonal Architecture

Hexagonal Architecture, or Ports and Adapters, was proposed by Cockburn (2005) @cockburn2005. Like Clean Architecture, the main idea is to separate the application from the external world. There are two types of ports. Primary ports are driven by the external world, while secondary ports are driven by the application itself.

In the context of the Hexagonal Architecture pattern, the application should be equally testable and playable whether being driven by the user through the web interface, a test harness, or a bash script. Any interaction with the external world should go through these ports, allowing the application to easily substitute any adapter for another.

== Backend Technologies

=== C++ for Web Services

C++ is not usually used for web applications, where interpreted languages (Python, JavaScript) or garbage-collected compiled languages (Go, Java) are typically used. However, C++ has deterministic memory management that makes it appropriate for applications that require high performance @stroustrup2013. Furthermore, modern versions of C++ (C++17 and later) include features that improve developer productivity, such as std::optional, std::variant, std::expected (in C++23), structured bindings, and constexpr @cppreference2024.

In addition to the performance characteristics of C++, the language is also used in scientific data processing applications. For instance, C++ is used to implement high-performance scientific libraries that are used in astronomy, such as the CFITSIO library @pence2010.

=== Drogon Web Framework

Drogon is a C++17/20 HTTP application framework @drogon2024. It supports HTTP/1.1 and HTTP/2, WebSocket, and features an ORM to access databases. Drogon employs a multi-threaded event loop architecture similar to Node.js, but using OS threads. Each thread features its own event loop to receive incoming connections. The non-blocking I/O operations, such as database queries or HTTP requests to remote endpoints, are performed asynchronously to improve application performance.

Drogon features a filter system that allows developers to insert code that should execute for every request before it reaches the application’s controllers. Filters are most useful for tasks that apply to all requests, such as authentication, logging, or validating CSRF tokens. Drogon’s controllers are classes whose methods are annotated with macros to map them to HTTP routes. Drogon features an ORM that supports PostgreSQL, MySQL, and SQLite databases with both synchronous and async database access.

== Database Design for Time-Series Data

=== PostgreSQL for Scientific Data

PostgreSQL is a popular open-source database system that is frequently used in scientific contexts @postgresql2024. Features relevant to astronomical databases include UUID primary keys, TIMESTAMPTZ, DOUBLE PRECISION fields, partial indexes, and the ON CONFLICT clause. For measuring time-series data for objects, the database must be able to efficiently retrieve all data for a given observation. This type of query can be efficiently handled by creating a composite index on the objects’ primary key and the time data fields. Additionally, since astronomical data is often being read and written to simultaneously, PostgreSQL’s MVCC system ensures that reads and writes do not interfere with each other @postgresql2024.

=== Alternatives Considered

TimescaleDB is a PostgreSQL database extension specifically designed for time-series data, with features like automatic partitioning of tables according to time intervals (hypertables), data retention policies, and time-series optimised aggregation functions @timescale2024. While these features allow TimescaleDB to handle millions of data points effectively, the same level of performance is available from standard PostgreSQL for datasets of 10,000 to 50,000 data points per light curve.

InfluxDB does not support storing non-time-series data and would require implementing a polyglot database solution with additional operational overhead @kleppmann2017.

== Authentication and Security

=== JWT-Based Authentication

JSON Web Tokens (JWT) facilitate stateless authentication by having the server create a token that contains the user’s information and has a digital signature that the client includes with each request @jones2015. The server can validate this signature without accessing a database to check for login credentials. A JWT consists of three parts: a header, a payload containing claims about the user, and a digital signature.

Because JWTs are stateless, the server has no way of knowing if a token has been issued or is still valid. To combat this, services use a system of two tokens. The short-lived access token is used to authenticate the client with the API and is typically valid for only 15 minutes. The refresh token is valid for a longer period, typically 7 days, and is stored on the server @jones2015. With each use of the refresh token, a new one is created and the old one is invalidated. This ensures that if one of the tokens is used to access the API after it has been revoked, all tokens are invalidated @lodderstedt2020.

=== OAuth 2.0

OAuth 2.0 is an authorisation framework that allows third-party applications to gain access to a user's account on another service @hardt2012. The most common flow, the Authorization Code Grant flow, requires four steps to gain access to the user's protected resources. The user visits a frontend application, which redirects the user to an identity provider. Upon authentication by the user, the identity provider redirects back to the client application with an authorization code. The authorization code is sent to the backend application, which redirects back to the identity provider with a request for an access token and an ID token. The backend application decodes the ID token to access user information.

OpenID Connect is built upon OAuth 2.0, and returns a JSON Web Token (JWT) within the ID token. This token contains claims regarding the user that was authenticated. If this token is received directly from the identity provider's servers over HTTPS, it is unnecessary to verify the signature of the token; HTTPS ensures that the token could not have been sent from any other source @openid2014.

=== CSRF Protection

If the authentication state is carried within cookies (such as with HttpOnly refresh tokens), then the application is vulnerable to CSRF attacks by a malicious website @owasp2024. The double-submit cookie pattern helps to mitigate these attacks by having the server generate a random token to be submitted with requests, which is sent within the cookie and within the response body to the client's browser. For requests from the client application, the token is sent within a custom request header. The server compares the value of the cookie to the value of the custom header; because the attacker does not have access to the victim’s cookie, they will not be able to create a request with the appropriate header @owasp2024.

For security, the comparison should utilize constant-time comparison functions to prevent timing attacks that could allow the attacker to gain the value of the token by guessing characters of the token @bernstein2012.

=== Password Hashing

For securely storing passwords, a memory-hard hashing function must be used. Argon2, the winner of the Password Hashing Competition in 2015, offers three variants: Argon2d is a memory-hard function with data-dependent memory accesses that are resistant to GPU attacks; Argon2i uses data-independent memory accesses that are resistant to side-channel attacks; and Argon2id, a hybrid function of the other two, is recommended for password hashing @biryukov2016. Argon2id with at least 64 MB memory, 3 iterations, and 1 degree of parallelism is the standard recommended by the Open Web Application Security Project (OWASP) for hashing passwords @owasp_password2024.

== API Design

=== REST Principles

Representational State Transfer (REST) was defined by Fielding (2000) in his doctoral dissertation @fielding2000. The architectural style defines six constraints: separation of concerns between clients and servers, statelessness, cacheability, use of a uniform interface for network protocols, implementation of a layered system architecture, or enabling code-on-demand from server to client. RESTful APIs map different HTTP methods to different operations on the resources defined by the URLs within those requests: GET for retrieving representations of resources, POST for creating resources, PUT or PATCH for updating resources, and DELETE for removing resources from the servers.

Well-designed RESTful APIs use versioning within their routes (@fielding2000), use plural nouns for the names of the resources they expose, and return responses in a consistent envelope. Errors in those responses should use error codes, error messages, and optionally field-specific validation errors to allow clients to handle those errors appropriately.

=== Cursor-Based Pagination

For list endpoints that might return large result sets, cursor-based pagination has some advantages over offset-based pagination. Offset-based pagination can suffer from performance issues with larger data sets @winand2012. Each time a request is made for a page of results with an offset, the database must access the data set to find the offset value and then scan forward to the requested page of results. With cursor-based pagination, the cursor value can be used to directly navigate to the page of results via the database index; the requests will always take the same amount of time, regardless of how deep into the data set the cursor value is.

== Frontend Technologies

=== Next.js

Next.js is a React-based framework that provides server-side rendering, static site generation, file-system based routing, and API routes @nextjs2024. By rendering pages on the server, the content is instantly available for users and can be indexed by search engines.

Next.js 13 introduced a new router called the App Router that uses React Server Components as the default component type. These components are rendered on the server to decrease the amount of JavaScript that needs to be downloaded by the browser and to increase rendering performance on the server. Components that require access to the client side of the application are marked with the “use client” directive.

=== Data Visualisation

Interactive visualisation of tens of thousands of scientific data points within the browser requires the use of a library like D3.js, the foundational library for data visualisation @bostock2011. However, D3’s use of SVG elements to bind data to visual elements of the visualisation degrades in performance with more than approximately 5,000 data points.

For visualising large time-series data points, Canvas and WebGL are the preferred technology bases for visualisation libraries. Chart.js, a Canvas-based library, and Plotly.js, a library built on D3 but with WebGL support for visualising scatter plots, offer the required performance for interactive visualisation of tens of thousands of data points with zoom, pan, and tooltip interactions @plotly2024. WebGL can render hundreds of thousands of data points without any performance degradation, meeting the requirements for displaying full TESS light curves.

== Summary

The literature establishes several points about the project. The platform will produce well-documented and publicly available time-series data that adheres to the standard FITS format and can be accessed via the MAST API. Tools available to pull the same type of data require programming knowledge, have limited features, or are geared towards professional astronomers.

The technical literature discusses the use of a clean or hexagonal architecture, PostgreSQL databases, JSON web tokens for authentication, and ETL processes for data ingestion. These technologies will form the technical components of the project that will be discussed in the following chapter.
