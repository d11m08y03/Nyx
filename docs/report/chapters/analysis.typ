= Requirements Analysis <analysis>

In this chapter, we define the requirements for the Nyx platform. The requirements are derived from the problem statement defined in @introduction and the gaps in the literature reviewed in @lit_review. Furthermore, a stakeholder analysis is performed to determine which individuals will utilise the platform and what their needs are of the platform. The functional requirements are defined within seven subsystems of the Nyx platform. Beyond the functional requirements for the platform, there are also non-functional requirements for the platform. Additionally, the data requirements for the Nyx platform are defined. Use cases for the Nyx platform are also defined. Finally, the requirements are also provided with a MoSCoW analysis and traceability matrices.

== Stakeholder Analysis <stakeholder_analysis>

The identification of stakeholders is a prerequisite for requirements elicitation. A stakeholder is any individual or group that has an interest in the system, is affected by the system, or that can influence the system @cockburn2005. Nyx serves a niche market of users that includes amateur astronomers from Mauritius, professional astronomers and educators, and the system administrator.

=== Amateur Astronomers in Mauritius

The audience that will be targeted for Nyx is amateur astronomers based in Mauritius. Mauritius is located in the southern hemisphere at 20.3 degrees south latitude of the equator, which provides observations of objects of the night sky that are inaccessible from most observatories in the northern hemisphere. Furthermore, its longitude of 57.5 degrees east of the prime meridian places it in a geographical location in between most other observatories across the planet @hessman2004. Despite these geographical advantages for astronomy in general, however, there is no existing astronomical infrastructure on the island.

There are several pain points for amateur astronomers in Mauritius in particular. For instance, the available astronomical data from the Transiting Exoplanet Survey Survey (TESS) requires the formation of API requests in JSON format and binary FITS files to access the data for analysis @stsdci2024. Furthermore, there is no available platform for amateurs to upload their observations of targets in the sky. Additionally, the standard time systems for astronomical observations on the ground (Universal Coordinated Time (UTC), or Julian Date) do not match the time system utilized by TESS for its astronomical measurements known as Barycentric TESS Julian Date, which requires a time conversion process for each observation of an object @eastman2010. Finally, there is no method for amateur astronomers in Mauritius for recording which astronomical equipment was used to perform each set of observations.

The goals for this audience will be to able to search for astronomical objects by their common names without needing to be familiar with the various astronomy and sky catalogue identifiers for those objects. Furthermore, they will want to view the TESS light curves for these objects of interest. Additionally, they will want to be able to upload their observation images to Nyx and automatically have the metadata for those images extracted. Furthermore, they will want to be able to overlay their ground observation measurements onto the TESS light curves of those same objects. Finally, they will want to be able to maintain a record of the equipment used to perform their astronomical observations.

=== Professional Researchers and Educators

While Nyx is not designed to replace professional research tools such as Lightkurve @lightkurve2018 or the MAST web portal @stsdci2024, professional astronomers and astronomy educators represent a secondary stakeholder group. Researchers who coordinate follow-up observations of TESS Objects of Interest may use the platform to quickly verify whether an amateur's observation is consistent with the expected transit signal. Educators may use the platform as a teaching tool, demonstrating how space-based and ground-based photometry differ in cadence, precision, and noise characteristics.

The pain points for this group are different from those of amateurs. Researchers need reliable, well-documented data provenance: they need to know exactly which TESS sector and observation a light curve came from, what quality flags were applied, and whether the flux values represent SAP or PDCSAP measurements. Educators need a platform that is accessible to students without requiring them to install Python environments or learn the MAST API. Both groups require the system to present data accurately, with correct time systems and flux units, and to make clear the provenance of every data point displayed.

=== System Administrator

The system administrator will be responsible for deploying, configuring, monitoring, and maintaining the Nyx platform. The system administrator desires that the platform is configurable without recompilation, produces logs that can be monitored, and handles errors gracefully.

The pain points for the system administrator relate to the failures of the system. The system should not fail without notice of unavailable dependencies. Any errors messages should contain information that can help with troubleshooting, and the system should be able to be deployed on a Linux machine. The system should be able to handle concurrent users without corrupting the data, should have the ability to rate limit access to authentication endpoints to prevent brute force attacks, and should not expose internal error messages to end users.

The goals of the system administrator are to deploy and configure the system, monitor the system logs to ensure everything is operating normally, maintain the security of the system, and ensure that concurrent users will not corrupt the data.

=== Stakeholder Summary

The three different stakeholder groups, their goals, pain points, and how Nyx addresses each of these pain points has been described in the subsections that precede this section. The amateur astronomy audience in Mauritius is primarily targeted by Nyx, and Nyx addresses their needs through automation of steps like accessing the MAST API through a search bar, automatically downloading and parsing the resulting FITS files, uploading FITS files and extracting their EXIF data, and providing interactive sky charts. Professional researchers and educators will find value in the software due to the availability of the full TESS observation data in the database, the use of a web interface that does not require installation on the users’ computers, and the availability of the SAP and PDCSAP flux measurements of the observed stars. Finally, the system administrator will find the software to be secure through the use of environment variables, spdlog logs with correlation IDs, Argon2id hashing with JWT authentication and rate limiting, and the use of PostgreSQL’s MVCC protocol to ensure data access safety.


== Functional Requirements <functional_requirements_section>

The functional requirements are organised into seven subsystems. Each requirement has an identifier (FR’X’.Y) where X is the subsystem number and Y is the requirement number within that subsystem. Each requirement is described in detail to allow for implementation and testing.

=== FR1: Authentication and User Management

The authentication subsystem allows users to register for and subsequently log into the application. Because the authentication subsystem is a prerequisite for all other subsystems, only users who have successfully logged into the application can access the features provided by those other subsystems. The authentication system employs the strategy of utilizing both JWT tokens with the refresh token rotation strategy as described in @lit_review, as well as requiring users to verify their email addresses when they register for the application.

FR1.1 allows users to register for the application. When a user performs this action, the user is required to provide their email address and password. The system validates that the email address is appropriate for the application (has the correct format and is not already in use) and that the password is of an appropriate complexity before proceeding with registration. If the registration information is valid, a user is registered in the system with their password hashed with Argon2id, an email verification token is generated, and an email is sent to the user with the verification link. The user is not permitted to login until they have verified their email address.

FR1.2 enables users to login to the application. A user provides their email and password upon attempting to login. The application checks to ensure that the user with that email address is validly registered in the system, that their email address has been verified (FR1.5), and that the provided password matches the hashed password associated with their user account. Upon successful login, both a short-lived JWT access token and a refresh token are issued to the user according to the specification described in NFR3. Additionally, any failed attempts to login are logged with the IP address of the user attempting to gain access.

FR1.3 enables users to login to the application using their Google account. The OAuth2 implementation utilizes the Authorization Code Grant Flow as described in @hardt2012. Thus, when a user selects to use their Google account to login to the application, the frontend system redirects the user to Google’s authentication screen. Following authentication by the user, Google redirects the user to the frontend with the authorization code. The authorization code is sent to the authentication subsystem’s backend server where it is used to obtain an ID token that contains the user’s email address and Google ID as described in @openid2014. If there is no user with that email address in the system, one is created automatically with its email address verified. If there is an existing user with that same email address but who created their account locally, their Google account is linked to that existing user.

FR1.4 enables users to verify their email address. When a user registers for the application, a long-lived, cryptographically random token is generated. The token is hashed with SHA-256, and that hashed value is stored in the system with a expiry date. Additionally, the original, unhashed token is sent to the user through the verification email. When the user clicks on the link included in that verification email, the application hashes the verification token that is provided by the user, compares that hash to the stored hash, and ensures that the verification token has not expired. If all of these parameters are satisfied, the user’s account is marked as verified in the system, and their verification token is automatically deleted.

FR1.5 prevents unverified users from logging into the application. When a user attempts to login but whose account is unverified, the login process will reject the user and inform them of the requirements to verify their email address.

FR1.6 allows users to request that a verification email be resent. A user can request the re-sending of a verification email by providing their email address to the system. The system validates that the user exists in the application and that they have not already verified their email address. If these conditions are satisfied, the system automatically invalidates the existing verification token for that user, generates a new one, and sends the user a new verification email.

FR1.7 enables the application to utilize short-lived access tokens and long-lived refresh tokens for users of the application. Additionally, refresh tokens are automatically rotated when they are obtained. When a user presents their valid refresh token to the authentication server to acquire a new access token, the authentication server issues a new access token and refresh token to the user, invalidating the refresh token that is presented to it. Each refresh token is associated with a family of tokens, identified by a family ID. Thus, if a refresh token is found to have been invalidated, all of the tokens within that refresh token’s family are automatically invalidated as well @lodderstedt2020.

FR1.8 enables users to log out of the application. When a user logs out of the application, the system revokes the user’s current refresh token, clears the user’s refresh and CSRF tokens in their browser cookies, and returns a success message to the user. While the user will still be able to use their access token until it naturally expires after 15 minutes, they will no longer be able to use their refresh token to obtain new access tokens after logging out.

As a system, the authentication system is critical to the operation of the application. Without the ability to successfully register for (FR1.1) and login to (FR1.2) the application, no users will be able to use the remaining features of the application. Additionally, the Google OAuth2 login functionality (FR1.3) is an added convenience for users who may be more accustomed to utilizing their Google account than to having to establish a local user account. The requirement for users to verify their e-mail addresses (FR1.4) ensures that the e-mail addresses registered to the application are those of genuine users of the application, which is required for implementing a password reset system in the future. Additionally, the requirement for refresh tokens to be automatically rotated when they are issued (FR1.7) limits the potential damage should a refresh token become compromised @lodderstedt2020. Finally, the implementation of a logout function (FR1.8) is both a feature that users will expect to be available, as well as one that is required to complete the authentication lifecycle.

=== FR2: Target Management

The target management subsystem is responsible for resolving target names, retrieving TESS observation metadata from the MAST archive, discovering FITS data products associated with those observations, and downloading and parsing those FITS files to extract light curve data. This subsystem forms the data processing pipeline for Nyx, distinguishing the platform from existing tools.

FR2.1 Target Name Resolution: Users can search for an astronomical target by its name, such as \texttt{pi Mensae} or \texttt{HAT-P-7}. The target name is sent to the MAST API via its \texttt{Mast.Name.Lookup} service. The service returns a response that includes the name of the astronomical object, its RA and Dec coordinates, and a \texttt{canonical_name} that can be used to refer to the target in the remainder of the MAST API. These response fields are stored in the database. If the target has been previously looked up by this subsystem, the lookup result is retrieved from the database rather than resolving the target name through the API call to the MAST API.

FR2.2 Coordinate Caching: When an astronomical target is looked up (as in FR2.1), its RA, Dec, and canonical name are stored in the database’s \texttt{targets} table. Any subsequent lookups of that same target will return the target’s record from the database, skipping the lookup of its name through the MAST API. The cached coordinates will never time out; since the coordinates of a given astronomical object never change, there is no need for a time-out for the cached record.

FR2.3 TESS Observation Retrieval: Having looked up the astronomical target (FR2.1), the subsystem can query the MAST API for TESS observations of that target. Specifically, the subsystem makes a request to the \texttt{Mast.Caom.Filtered.Position} service of the MAST API, providing the RA, Dec coordinates of the astronomical target, and the name of the target. The TESS observations are returned as a list of observations that lie within a 0.005 degree radius of the target. Each observation includes the \texttt{obsid} of the observation, the \texttt{sequence} number of the observation, the \texttt{target_name}, the \texttt{start_time} and \texttt{end_time} of the observation, as well as a \texttt{dataURI} that can be used to retrieve the data products related to the observation. These observations are stored in the database in the \texttt{tess_observations} table. The \texttt{obsid} column of this table is used as the primary key for the table to enable idempotent operations on the observation; downloading the same observation twice will not result in the insertion of the same record twice into the database.

FR2.4 TESS Observation Listing: The subsystem can return a list of observations of the astronomical target that has been looked up. This list will have one observation record for each observation of that target that is stored in the \texttt{tess_observations} table. Each observation record will contain the \texttt{obsid}, \texttt{sequence}, \texttt{target_name}, \texttt{start_time}, \texttt{end_time}, and the \texttt{duration} of the observation (duration in seconds), as well as the \texttt{lc_points} for that observation. The \texttt{lc_points} value will be zero if the light curve for that observation has not yet been downloaded.

FR2.5 FITS Product Discovery: For a particular TESS observation, the subsystem can retrieve the list of FITS data products related to that observation. The list of related products can be obtained by making a request to the \texttt{Mast.Caom.Products} service of the MAST API. The response to this request will be a list of products related to the observation. The subsystem parses this list of products to find the FITS light curve file; this file will have a \texttt{.lc.fits} file extension. The \texttt{dataURI} of that FITS file is stored in the \texttt{tess_observations} table associated with the observation.

FR2.6 FITS Download and Parse: The FITS file can be downloaded from the MAST archive using the \texttt{dataURI} of the file. The file is parsed to extract the light curve data. The FITS file contains a binary table that stores the light curve data. The table has the following columns: the \texttt{TIME} of the observation, the \texttt{PDCSAP_FLUX}, \texttt{SAP_FLUX} of each pixel of the TESS detector, and the \texttt{QUALITY} of each flux measurement. The parser of the FITS file uses the \texttt{TIME}, \texttt{PDCSAP_FLUX}, \texttt{SAP_FLUX}, and \texttt{QUALITY} columns (@pence2010). The time values are stored as a double-precision floating-point number, the flux values are stored as double-precision floating point number, and the quality value of each pixel is stored as a 32-bit integer. Additionally, TESS light curves may contain \texttt{NaN} values for flux measurements; these values are ignored by the FITS parser (@tenenbaum2018). Furthermore, the FITS file is parsed using raw binary data and pointer arithmetic to access the relevant fields in the file; the CFITSIO library is avoided to minimize the number of external libraries required to compile Nyx.

FR2.7 Light Curve Storage: The light curve points that are parsed from the FITS file are stored in the database. Each record in the table \texttt{light_curve_points} contains the \texttt{TIME} of the observation as a double-precision floating point number, the \texttt{PDCSAP_FLUX}, \texttt{SAP_FLUX} values of the pixel in the detector, and the \texttt{QUALITY} of that flux measurement. The record also has a foreign key that links the point to the TESS observation from which the point was derived. An index on the \texttt{tess_observation_id} and \texttt{time} fields of the \texttt{light_curve_points} table enables fast retrieval of the points for a given observation. The points are inserted into the table in bulk to minimize the number of database round trips. Should the subsystem determine that there are already light curve points for that TESS observation in the database, those points are first deleted before the bulk-insertion of the newly parsed points.

The target management subsystem represents the major distinction of Nyx from existing tools for retrieving TESS light curve data. Existing tools require that users either directly access the MAST API, or write Python scripts to retrieve the light curve data from MAST @lightkurve2018. The target management subsystem of Nyx removes this barrier to accessing the TESS data for amateur astronomers. Furthermore, each step of the data retrieval process can be cached independently. For instance, the coordinates of a target are cached with a permanent time-out (FR2.2), the observations of a target are cached with a time-out related to the ingestion of new targets (FR2.3), and the light curves of the observations are cached with a time-out of the first ingestion of a target’s observations (FR2.7).

=== FR3: Equipment Management

The equipment management subsystem allows the user to maintain a catalogue of their astronomical equipment. This catalogue can then be associated with the observation sessions (FR4) to provide context for the observation regarding the equipment that was used during those observations.

FR3.1 Telescope CRUD: Users can create, read, update and delete telescope objects. Each telescope will have a name that is unique to the user, an aperture in relation to the amount of light that can be gathered by the telescope (in relation to its diameter in millimetres), and the focal length (in relation to the distance of the telescope’s mirror or lens to its point of focus and how wide the telescope’s field of view is) @gary2007.

FR3.2 Camera CRUD: Users can create, read, update and delete camera objects. Each camera object will have a name that is unique to the user, a sensor type (either CCD or CMOS sensors) that influence the amount of noise in the image that is captured by the sensor @conti2018, and a resolution of the sensors in megapixels.

FR3.3 Mount CRUD: Users can create, read, update and delete mount objects. Each mount object will have a name that is unique to the user, and the type of mount (equatorial, alt-azimuth, Dobsonian). Equatorial mounts allow for the telescope’s field of view to be tracked in relation to the movement of celestial objects in the sky, while alt-azimuth mounts must compensate for field rotation in long-exposure observations. These mounts impact the length of exposure that can be made with the telescope.

FR3.4 Filter CRUD: Users can create, read, update and delete filter objects. Each filter object will have a name that is unique to the user and of a given type (Johnson V, Johnson B, Sloan g', luminance, no filter). Using different filters allows for ground-based telescopes to observe in different wavelength bands of the electromagnetic spectrum. For instance, the TESS telescope captures all light within a broad red portion of the spectrum (between 600 and 1000 nm in wavelength) @ricker2015.

While the equipment management subsystem is somewhat simpler than the others, it is still essential to provide context for the observations. Without knowing the type of equipment used by an astronomer to perform their observations, it is impossible to understand why their observations may differ from those made by the TESS telescope. Furthermore, the equipment type has a unique constraint upon the (user_id, name) of each type of equipment to avoid adding entries for the same object under different user IDs or different names within the same user’s database.

=== FR4: Observation Management

The observation management subsystem allows ground-based observation sessions to be recorded (with associated equipment and location records), and for the images from those observations to be uploaded and processed.

FR4.1 Observing Location CRUD: Users can create, read, update, and delete observing location records. Each location record has a name (which is required to be unique to each user), latitude in decimal degrees, longitude in decimal degrees, and altitude in metres above sea level. These geographic coordinates are required for time system conversions to convert between, for instance, UTC/JD (used in ground-based observation) and BJD (used in TESS data), @eastman2010. All users in Mauritius are assigned a default observing location: latitude -20.3, longitude 57.5.

FR4.2 Observation Session Creation: Users can create observation sessions that record the details of a single observation session for a specific astronomical target. Each observation session record is associated with the astronomical target that was observed (via link to the target record from FR2.1), the location from which the observations were made (via link to the location record from FR4.1), and the observing equipment that was used (via link to equipment items from FR3.1-FR3.4). Each observation session record also includes a start time for the session and additional notelinenotes about the observations made during that session. Those observation sessions are the link between observation images and the astronomical targets that those images were of.

FR4.3 Image Upload: Users can upload observation images to an observation session. Supported file types are JPEG, PNG, FITS and DNG (Digital Negative) files. Uploaded files are stored on the server and their filename is set to a generated value. The observation session record in the database is updated with the file path to the uploaded file, the original filename of the file, the size of the file in bytes, and the MIME type of the file. Each observation session can contain multiple observation images.

FR4.4 EXIF Metadata Extraction: When a JPEG or PNG image is uploaded to an observation session (as defined in FR4.3), the system will automatically extract the EXIF data from the image file. Such metadata includes the manufacturer and model of the camera that captured the image, the length of time that the image was exposed to light (in seconds), the sensitivity of the camera’s sensor to light (ISO), the date and time that the image was taken, the focal length of the camera lens that took the image, and the geographic coordinates of the location from which the image was taken (if the image contains that information). This extracted metadata is stored in a JSON field within the observation session’s record in the observation_images table. By automatically extracting this information, observations can be recorded without requiring users to manually enter these parameters for each observation.

The observation management subsystem is the counterpart to the target management subsystem; while subsystem FR2 handled data from space-based telescopes like TESS, subsystem FR4 handles data from ground-based observations from the user’s own telescope. More specifically, the linking of observation sessions to astronomical targets (as described in FR4.2) is what enables the implementation of the comparison visualisation described in FR5.3; without establishing these links between observation and target records, the software would not be able to associate any ground-based observations with the targets from TESS. Furthermore, the implementation of automatic extraction of exposure parameters (as described in FR4.4) is beneficial because it allows the system to automatically record those parameters at the time the observation was made (rather than requiring the users to manually enter those values after the fact).

=== FR5: Data Visualisation

The data visualisation subsystem provides the frontend components that are required for displaying the data that has been stored in the backend. The subsystem transforms the raw data from the backend into visual representations that allow the user to analyse the data.

FR5.1 requires the frontend to display the light curve data in the form of interactive scatter plots. Time is represented on the horizontal axis of the chart, and flux is represented on the vertical axis. Each point on the plot represents a data point from the TESS mission. The plot should display PDCSAP flux measurements as the default setting, but allow users to select the display of SAP flux measurements. The frontend must be capable of displaying up to 50,000 data points before any lag is introduced to the display of the plot. Implementing SVG graphics libraries will not meet this requirement, and Canvas or WebGL based graphics libraries should be used instead @plotly2024.

FR5.2 requires the light curve charts to allow the users to zoom into specific portions of the light curve for closer viewing. Zooming into specific portions of the light curve can be performed using the scroll wheel on a mouse or touch gestures on a touch device. Panning the light curve to view different portions of the plot can be performed by clicking and dragging on the plot. The axis labels and tick marks will adjust to the portion of the plot that is zoomed in on. Additionally, there should be a button on the plot that resets the display to the entire light curve. Such interactions are essential for being able to examine specific features of a full-length TESS light curve that can take up to 27 days to complete and contain tens of thousands of data points @ricker2015.

FR5.3 requires the frontend to allow for the overlay of the ground-based observation data on the TESS light curve. The ground-based data displays with a different colour and marker shapes to indicate that they are not TESS observations. By displaying both datasets on the screen simultaneously, the user can compare their ground-based observations of the same object to the observations made by TESS. The ground-based data will be displayed in terms of relative flux only, as the flux measurements from these two data sources are not normallyised to the same scale.

FR5.4 requires the frontend to provide a searchable catalogue of the astronomical objects that have been resolved in the system. The catalogue will display the name of the object, the type of object, its coordinates in the sky, and the number of TESS observations of that target. The user will be able to search for objects by name, and any row can be sorted by any of the displayed fields. Clicking on an object will navigate the user to a page containing details about the object, a list of its TESS observations, and the light curves of those objects.

As the primary means of deriving value from the Nyx platform, the data visualisation subsystem is essential to the platform’s value proposition. The value proposition of the Nyx platform is to provide access to TESS data to amateur astronomers, and to allow those amateur astronomers to compare such measurements to their own ground-based observations of those same objects (FR5.1, FR5.2, and FR5.3). Without the data visualisation subsystem, the backend and ingestion subsystems (FR2) and the observation management subsystem (FR4) will have produced data that cannot be accessed or utilised by the user. The requirement for 50,000 data points to be displayed (FR5.1) can be derived from the typical length of a TESS light curve. A full sector of TESS has a 2-minute cadence and observes for 27 days. During that time, TESS performs approximately 19,400 observations per sector. It is possible for an object of interest to be observed in multiple sectors during these 27 days. Thus, 50,000 data points is the rounded up value of the observations per 27 days @ricker2015.

=== FR6: Data Pipeline

The subsystem that manages the execution of the ingestion tasks must manage tasks that may take several seconds to complete. While the resolution of targets (FR2.1) and observation search (FR2.3) tasks may be completed in sub-second time, the tasks of performing product discovery on TESS observations (FR2.5) and downloading and parsing light curves (FR2.6) can take several seconds due to the nature of downloading data from the MAST archive. The execution subsystem for the application allows for these tasks to be executed while reporting to the user about their status.

FR6.1 Asynchronous Ingestion: Tasks that take several seconds to complete (such as discovering light curves and parsing them from their FITS files) will execute in the background of the web application. When a user begins ingestion of data from a TESS observation, the system will return an HTTP response indicating the ingestion has begun, but will execute the task in a background thread to allow for ingestion to begin without time-outs on the HTTP requests @drogon2024.

FR6.2 SSE Progress Reporting: The system will report the status of an ingestion task to the user through Server-Sent Events (SSE) @w3c_sse2015. Each event will describe the stage of the ingestion task (such as discovering products, downloading the FITS file, parsing the light curve, storing the data points) and whether the task is completed. The frontend of the system will use the EventSource API to subscribe to these events @w3c_sse2015. Using Server-Sent Events instead of WebSockets is driven by the requirement that the communication is unidirectional (from the server to the client only); additionally, using standard HTTP is beneficial due to the ease of proxy and firewall configurations @w3c_sse2015.

FR6.3 Idempotent Re-Ingestion: Attempting to ingest the same target or observation will not result in the creation of two separate records of the same information. Records of TESS observations include a unique constraint on the MAST obsid. Additionally, when re-ingesting observations, the system uses the ON CONFLICT DO NOTHING statement to exclude any data that may already exist @postgresql2024. For individual points of light curves, if the system re-ingests an observation, the existing points will be first deleted, then the system will re-insert the newly-parsed light curve data. Idempotency ensures that re-running the ingestion task (whether as a result of an intentional re-run or due to system failures during ingestion) will not result in data corruption of the database @kleppmann2017.

The pipeline task execution subsystem helps to address the difficulty of implementing a web application that calls external APIs that may take several seconds to complete the requested tasks. Without asynchronously executing the tasks (FR6.1), it would be impossible to complete the HTTP requests and receive a response from the web server. Additionally, without reporting the status of ingestion tasks to the frontend (FR6.2), the user would be unaware of the status of the system’s tasks. Finally, without ensuring that re-running the ingestion tasks will not lead to data corruption (FR6.3), the database could become corrupted or otherwise contain data that is no longer accurate or reliable.

=== FR7: Image Processing and Photometry

The image processing subsystem is responsible for performing photometry on ground-based observation images. While the TESS data is pre-processed such that it is published as flux measurements within FITS files, the ground-based observations must have flux measurements extracted from the raw images via aperture photometry. Thus, this subsystem performs the necessary functions to transform raw observation images (FR4.3) into flux measurements (FR5.3) that can be compared with TESS data.

FR7.1 EXIF Metadata Extraction: The system can extract EXIF metadata from the JPEG and PNG observation images that are uploaded to the database. Such metadata includes information about the observation device (camera make and model), the exposure settings (aperture, shutter speed, ISO), the date and time stamp of the observation, the focal length of the device was used, and GPS coordinates of the observation location (where available). This information is stored in the database as a JSON object associated with the observation image record. This extraction occurs automatically upon the upload of the observation images (FR4.4).

FR7.2 DNG Raw Image Decoding: Raw images in the DNG (Digital Negative) format can be decoded on the system. DNG files are raw files that are used by astrophotographers to store direct data from the camera sensor in uncompressed format, which allows for greater dynamic range during image processing @conti2018. The raw data is decoded into a pixel array for use by the system. Raw observation images are required to be decoded prior to performing aperture photometry on those raw observation images (FR7.3).

FR7.3 Aperture Photometry with Source Detection: Aperture photometry can be performed on the decoded observation images. The system first determines the approximate location of the target star within the observation image. The system then detects the exact center of the observation source within the image, and determines the centroid of that source within the image @conti2018. Using this detected center, the system creates an aperture of a selected radius around that detected source, creates an annulus outside that aperture to measure background flux, determines the total flux within the aperture, subtracts the background flux, and reports the resulting flux measurement of the detected source. The source detection algorithm can determine the centroid of the source to sub-pixel precision for accurate aperture placement @conti2018.

FR7.4 Flux Measurement with Error Estimation: The flux measurement can also be accompanied by an estimate of the error in that measurement. For example, the error can be determined through calculations of the uncertainty in the photon count of the target star (Poisson statistics), background noise (from the background annulus pixel values), and the noise in the readout of the camera sensor @gary2007 @ricker2015. This error estimate is stored together with the flux measurement, which can then be utilized by the visualisation subsystem (FR5.3) to visually represent the measurement errors of the ground-based observations.

The image processing subsystem represents the most technically demanding subsystem within the ground-based observation system.  Without the ability to perform photometry on those images (FR7.3), the raw observation images are merely pictures of the stars of interest.  Furthermore, the measurement errors within FR7.4 are particularly important for ground-based observations.  Ground-based observation images tend to contain a higher level of noise than the measurements provided by TESS.  For instance, amateur astronomers using ground-based telescopes typically achieve only 1-5% precision in their photometry measurements of stars, while TESS achieves sub-millimagnitude precision measurements for the same stars @gary2007 @ricker2015.  Thus, without error measurements for ground-based observations, amateur astronomers may incorrectly believe that their ground-based observations reflect genuine signals from those stars being observed.


== Non-Functional Requirements <nonfunctional_requirements_section>

Non-functional requirements define the quality attributes that the system should exhibit. Unlike functional requirements, which define what the system will do, non-functional requirements define in what way the system should perform those functions. The non-functional requirements for Nyx include requirements relating to performance, security, reliability, maintainability, and operational concerns of the system.

NFR1: API Response Time - API calls to data that is cached will return within 200 milliseconds. API calls that require additional calls to external APIs (like MAST) will not be included in these response time requirements. Such a response time is required because 200ms is the threshold at which human users can begin to notice delays in a system’s responsiveness.

NFR2: Password Hashing - All passwords will be hashed using Argon2id, with at least 64 MB of RAM allocated to hashing that password, 3 iterations of hashing, and with 1 degree of parallelism @owasp_password2024. This choice of hashing algorithm ensures that attackers can not perform brute force attacks on the system; with 64 MB of RAM allocated to hashing the password, an attacker with 64 GB of RAM can only perform around 1,000 password hash attempts at once, whereas unsalted SHA-256 can perform billions of password hash attempts per second @biryukov2016. Argon2id was chosen because it resists both GPU attacks (using Argon2d) and side channel attacks (using Argon2i).

NFR3: Token Expiry - Access tokens will expire after 15 minutes, and refresh tokens will expire after 7 days. Access tokens will be included in the API response’s JSON response body, which is stored in the client-side application in memory (rather than in localStorage to avoid being accessed by XSS attacks). Refresh tokens will be stored as an HttpOnly, Secure, SameSite=Strict cookie restricted to the /api/v1/auth API endpoint @jones2015. This strategy ensures that access tokens have a short lifespan (limiting damage if stolen), while allowing for convenience in the user experience (only refreshing after 7 days of inactivity).

NFR4: CSRF Protection - All endpoints that permit a user to make changes to the system will utilize the use of a CSRF double-submit cookie @owasp2024. When a user logs into the system, a random token will be generated by the backend. That token will be returned in both the cookie and the response body. The response body will be stored by the client-side application in memory. Any requests sent by the client will include this same token in an X-CSRF-Token request header. Any requests that do not include the correct token will be denied with a 403 Forbidden response code.

NFR5: Rate Limiting - Requests made to authentication endpoints will be limited to a maximum of 10 requests per minute per IP address. This limits potential threats to authentication endpoints, such as brute-force password attacks on users, and DoS attacks against the computationally-intensive Argon2id password hashing algorithm. Rate limiting is enforced on the backend in memory. Any requests that reach the rate limit will be denied with a 429 Too Many Requests response code.

NFR6: Consistent Response Envelope - All API responses will be in the same structure. Successful responses will include a data field with the content of the response and a meta field with information about the request. Error responses will have a response in the data field replaced by an error field that includes information about the error. This enables error handling on the client-side, and monitoring of API endpoint calls by automated monitoring tools @fielding2000.

NFR7: Error Information Hiding - Any internal errors of the server will not be made visible to the client applications. Internal error messages will not include details about the system’s internal structure (database queries, system tables, files, etc.). This prevents attackers from gaining knowledge of potential vulnerabilities of the system @owasp2024.

NFR8: Correlation ID Tracing - Every request to the backend server will have a unique correlation ID associated with it, which will be logged each time the request is processed by the server. This allows backend services to correlate incoming requests with server logs, which is helpful with identifying and fixing issues with those requests. This functionality is especially important for Nyx because of the need to trace requests to asynchronous services like MAST.

NFR9: Concurrent Access Safety - The system will be able to handle multiple users at the same time. Database access is performed by PostgreSQL using its MVCC model to ensure that multiple processes reading and writing to the same database tables will not interfere with each other’s processes @postgresql2024. Both write and read processes are handled by the Drogon web framework, which is able to handle high levels of concurrent requests made to the server @drogon2024. The Nyx application is stateless, except for its use of the database to store user information.

NFR10: Platform Compatibility - The backend APIs will be compiled and run on Linux using GCC 13+ or Clang 17+. Its dependencies are managed using CMake and vcpkg, both of which do not use proprietary libraries. The client application will be able to run in any web browser that supports ES2020 and the EventSource API @nextjs2024. This ensures that the backend server can be deployed on Linux servers, and accessed from any device using a modern browser.

NFR11: RAII Resource Management for FITS Files - All resources for FITS files will use RAII (Resource Acquisition Is Initialisation) to ensure that allocated resources (RAM) are released when the object using those resources goes out of scope @stroustrup2013. FITS files downloaded from MAST will be stored in a container of type std::vector<uint8_t>, whose memory is automatically released when the container goes out of scope. This prevents memory leaks, especially since FITS files can be tens of megabytes in size. RAII also ensures that the allocated memory is released when multiple FITS files are downloaded in a sequence.

NFR12: Batch Size Tuning for Bulk Inserts - Bulk inserts of data (light curve points) to the database will occur in batches rather than all at once. Using parameterised SQL statements, bulk insertions will be limited to a configurable batch size. The default batch size ensures that insertion times remain within a few seconds (rather than tens of seconds) for light curves with up to 18,000 points @winand2012.

The non-functional requirements for Nyx are just as important as the functional requirements for the system.  An API that takes several seconds to respond to requests will provide an unpleasant user experience; a system that does not appropriately secure user passwords is a liability to the organisation that deploys the system; a system that returns internal error codes in its API responses can present a security risk; a system that does not properly manage memory may crash under sustained use; and the non-functional requirements define the quality of the system that is ultimately required for Nyx to be a successful platform.


== Data Requirements <data_requirements>

The data to be handled by the Nyx platform is described in this section. An understanding of the data that will be stored is a prerequisite to the database schema design (@design) as well as to making effective decisions regarding indexing and storage of the data.

=== TESS Light Curve Data

The TESS light curve files are distributed in the form of FITS binary table files. Each file corresponds to a single target that was observed in a single sector of TESS (approximately 27.4 days of observation). The file contains multiple Header Data Units (HDUs). HDU 2 of the file contains the actual data @pence2010.

The relevant columns in the table files are:

TIME: The time column contains values of Barycentric TESS Julian Date (BTJD). BTJD is defined as the Barycentric Julian Date minus 2,457,000.0 days @eastman2010. This value is stored as a 64-bit floating point number with FITS format code D. Typical values for this column range from ~1325 to ~1353.  Any cadence with no flux measurement will have a NaN value for this column.

PDCSAP_FLUX: This column contains values of the Pre-search Data Conditioning Simple Aperture Photometry. This flux measurement is corrected by the SPOC pipeline for instrumental systematics @stumpe2012. This value is stored as a 32-bit floating point number with FITS format code E. Typical values for this column range from a few hundred to several million electrons per second.

SAP_FLUX: This column is similar to the PDCSAP_FLUX measurement except that the flux is not corrected for systematics. This value is stored in the same format as the PDCSAP_FLUX measurement.  By comparing the values of these two measurements, the systematics of the TESS camera can be determined.

QUALITY: This 32-bit integer value contains information on potential issues with that specific data point @tenenbaum2018.  For instance, bit 0 indicates that there was an attitude tweak to TESS’ pointing during that observation, bit 1 indicates a safe mode, bit 3 indicates a cosmic ray within the optimal aperture, and so on.  A quality of 0 indicates that there were no known issues with the observation.  Any non-zero quality values indicate that the data point should be treated with caution.

A typical TESS light curve file contains between 18,000 and 20,000 rows of data (27.4 days of observation / day / 720 cadences per day).  However, since NaN values are present in the flux columns for some observations, there are typically between 15,000 and 19,000 rows of usable data.  For stars that are observed in multiple sectors of TESS, the total amount of data can reach several hundred thousand rows.

=== MAST API Response Format

The MAST API returns data in a columnar JSON format, which is different from the row-oriented JSON format that is typical of REST APIs @stsdci2024. The response to a request to the API contains two arrays: an array of objects that describe the fields of the data (their names and types) and an array of rows. Each row is an array whose elements correspond to the values in each of the fields of the data for that row of the result table. For instance, if the lookup of a name returns information about the object, the fields array will contain fields describing values like ra, dec, obj_type, and canonical_name, and the data array will contain the resolved object data in the rows of that array.

The columnar structure of the response is efficient during data transmission, as the field names are only transmitted once with each row of data rather than being repeated with each row. However, row-oriented data structures are required by most databases for insertion into those databases. Additionally, the response to each request from the API also includes metadata regarding the request, such as the number of results of the query, the status of the query, and any error messages that were returned in the case that the query failed. These responses from the API must be parsed by the software, any errors handled, and the data transformed into objects of the application domain.

=== Ground-Based Observation Data

Data from ground-based telescopes enters the system as image files in one of four different file formats:

- JPEG files are compressed with EXIF data but are unsuitable for photometry due to image compression.

- PNG files are lossless compressed but do not contain EXIF data.

- FITS files are the standard file format in astronomy but are uncommon among amateur astronomers. However, they are the standard among amateur astronomers who take serious astrophotography.

- DNG files are raw files from Adobe cameras that contain raw sensor data. These files are increasing in use among amateur astrophotographers with astronomy cameras and even smartphone cameras.

For ground-based observations, the following metadata must be collected: time stamp (UTC), exposure time, ISO setting, and the name of the target object. The following metadata would also be helpful but not necessary: make and model of the camera, focal length, GPS coordinates, and temperature of the surroundings of the telescope.

=== Photometry Data

When aperture photometry is performed on the ground-based observation image (FR7.3), the following measurements are made on each source within the image:

- Raw flux: The sum of the pixel values of the source within the aperture, after subtracting the background flux; this value is in arbitrary units (ADU) or in units of electrons if the gain of the telescope was set to multiply the raw pixel counts by a factor to convert them to units of electron counts.

- Flux error: The uncertainty in the raw flux value of the source; this is calculated from the quadrature sum of the photon noise of the source, the background noise, and the read noise of the detector.

- Relative flux: The flux of the target star divided by the flux of a comparison star; this value cancels out the effects of atmospheric extinction. This quantity is most comparable to the TESS PDCSAP flux of the star.

- Aperture parameters: The radius of the aperture, the radius of the inner annulus, and the radius of the outer annulus; these are all measured in pixels. These values have to be reported with the flux measurements to other astronomers in order to permit for the measurements to be made with the same apertures.

=== Data Volume and Access Pattern Summary

The following summarises the data types, their formats, typical volumes, and the primary access patterns that must be supported efficiently.

*Target metadata.* Format: JSON from MAST. Typical volume: hundreds of bytes per target. Storage: PostgreSQL `targets` table. Primary access pattern: lookup by name; lookup by ID.

*TESS observation metadata.* Format: JSON from MAST. Typical volume: approximately 500 bytes per observation, with 1--20 observations per target. Storage: PostgreSQL `tess_observations` table. Primary access pattern: list by target ID; lookup by obsid.

*TESS light curve points.* Format: binary FITS. Typical volume: approximately 18,000 points per observation; 4 doubles plus 1 int per point (approximately 36 bytes); approximately 650 KB per observation. Storage: PostgreSQL `light_curve_points` table. Primary access pattern: range scan by observation ID, ordered by time.

*Observation images.* Format: JPEG/PNG/DNG/FITS. Typical volume: 1--50 MB per image; 1--100 images per session. Storage: filesystem. Primary access pattern: list by session ID; individual image retrieval.

*EXIF metadata.* Format: JSON. Typical volume: approximately 1 KB per image. Storage: PostgreSQL JSON column in `observation_images`. Primary access pattern: read with parent image record.

*Photometry results.* Format: numeric. Typical volume: approximately 100 bytes per measurement; 1--10 measurements per image. Storage: PostgreSQL columns in `observation_images`. Primary access pattern: read with parent image record; aggregate by session.

*Equipment metadata.* Format: JSON. Typical volume: approximately 200 bytes per item; 1--20 items per user per type. Storage: PostgreSQL equipment tables. Primary access pattern: list by user ID; lookup by ID.

*User accounts.* Format: structured. Typical volume: approximately 500 bytes per user. Storage: PostgreSQL `users` table. Primary access pattern: lookup by email; lookup by ID; lookup by Google ID.

Next is the Tess light curve points. Each TESS observation contains approximately 18,000 data points. If an object is observed in 10 different sectors of the sky, there will be approximately 180,000 data points for that object’s light curve. Thus, with 1,000 different targets and 5 observations of each target, the light curve points table will contain approximately 90 million rows. While this is within the capabilities of PostgreSQL, creating the appropriate index on the table will be necessary to ensure that each query returns the desired rows within the 200ms target of NFR1 @winand2012.


== Use Cases <use_cases_section>

The following section describes each of the use cases for the Nyx platform in detail. Each use case diagram includes a description of the actors for the system, the preconditions for accessing the described use case, the main flow of the use case, any alternative flows for non-standard scenarios, exception flows for errors in the system, and the postconditions for the use case. Each of these descriptions are written in the fully-dressed format recommended by Cockburn @cockburn2005.

@use_case_diagram illustrates the primary use cases for the two actor types: unauthenticated visitors and authenticated users.

#figure(
  rect(width: 100%, height: 200pt, stroke: 0.5pt)[
    #align(center + horizon)[_Use case diagram placeholder --- to be replaced with rendered diagram._]
  ],
  caption: [Use case diagram showing interactions for unauthenticated and authenticated users.],
) <use_case_diagram>

The system has two types of actors. Unauthenticated actors can perform registration (UC1) and login using Google OAuth (UC7). Authenticated actors can search for and resolve astronomical targets of interest (UC2), fetch light curves of those targets (UC3), perform ground-based astronomical observations of those targets (UC4), perform aperture photometry on astronomical targets of interest (UC5), and compare space- and ground-based observations of those targets (UC6). All of these use cases except for UC1 and UC7 require the user to be authenticated.

=== UC1: Register and Verify Email

The registration use case covers the full lifecycle from account creation through email verification to first login. This is the primary onboarding path for users who do not use Google OAuth.

*Actors*: Unauthenticated visitor, email system.

*Preconditions*: The visitor has access to the Nyx frontend and a valid email address.

*Main Flow*:

- The visitor visits the Nyx website and clicks the registration button.
- On the registration page, the visitor enters their email and a password that meets the minimum complexity requirements.
- The frontend sends a registration request to the /api/v1/auth/register endpoint.
- The backend validates the entered email and password. Both validations pass.
- The backend checks whether there is an existing account with the provided email. No existing account is found.
- The backend securely hashes the visitor’s password using Argon2id encryption.
- The backend creates a new user account in the database with the hashed password and sets their email verification status to false.
- The backend generates a verification token for the account, hashes the token with SHA-256, and saves the hashed token in the database.
- The backend sends a verification email to the visitor with a link that includes the original verification token.
- The backend responds to the registration request with a 201 Created status code.
- The visitor receives the verification email and clicks on the link with the verification token.
- The frontend extracts the verification token from the URL and sends a request to the /api/v1/auth/verify-email endpoint.
- The backend verifies the validity of the verification token by hashing it and looking it up in the database.
- The backend marks the visitor’s account as verified in the database and removes the verification token record.
- The backend responds to the request with a 200 OK status code.
- The visitor can now log in to the Nyx website with their account.

*Alternative Flow A --- Email Already Registered*:

At step 5, if the provided email address is already taken, the API will return a 409 Conflict response with an error code of EMAIL_ALREADY_EXISTS. The visitor will be informed that their email is already registered with the system and will be redirected to the login or password reset page.

*Alternative Flow B --- Verification Token Expired*:

If the verification token has expired (more than 24 hours since token generation), a 400 Bad Request response will be returned with the error code of TOKEN_EXPIRED. The user will be required to request a new verification email via the resend verification endpoint (FR1.6).

*Alternative Flow C --- Resend Verification Email*:

If the visitor does not receive the verification email or the token expires, the visitor will be redirected to the resend verification page where they will have to enter their email address once more to initiate the sending of a new token that invalidates the previous one.

*Exception Flow --- Email Service Unavailable*:

If the email sending service is unavailable at step 9, the backend server will log the error using the correlation ID to investigate the root of the problem, create the user account, and return a 201 Created response with a warning message that the verification email was not sent and the user must request a resend of the email. The account will remain in the unverified state.

*Postconditions*: A new user account exists in the database. If the verification succeeds, the account will be marked as verified and the user will be able to log in. However, if the verification has not yet occurred, the account will exist in the database, but login will be blocked until the user verifies their email address.

*Related functional requirements*: FR1.1, FR1.4, FR1.5, FR1.6.

=== UC2: Search and Resolve Target

This use case describes the interaction between the user and the application when the user wishes to search for an object by name.

*Actors*: Authenticated user.

*Preconditions*: The user is logged in with a valid access token. The MAST API is accessible from the backend server.

*Main Flow*:

- The user navigates to the target search page on the frontend.
- The user enters the name of the astronomical target to be searched for (e.g., pi Mensae).
- The frontend’s JavaScript code sends a POST request to the /api/v1/targets/resolve endpoint, with the name of the target to be searched for in the request’s request body, and with the user’s access token and CSRF token included in the request’s headers.
- The JwtAuthFilter receives the request and validates the user’s access token.
- The CsrfFilter receives the request and validates the CSRF token.
- The TargetController receives the request and calls the resolve_target() method of the TargetService.
- The TargetService checks whether there is a record in the targets table with the target’s name. There is no record with that name in the table.
- The TargetService calls the resolve_target() method of the MastClient implementation of the IMastClient interface.
- The MastClient implementation of the IMastClient interface sends a POST request to the MAST server at the URL https://mast.stsci.edu/api/v0/invoke, with the service field set to Mast.Name.Lookup and the target name field set to the name of the target to be looked up.
- The MAST server returns the canonical name, right ascension, declination, and object type of the astronomical target in JSON data in a columnar format (i.e., a format in which the data is returned as comma-separated key-value pairs).
- The MastClient implementation of the IMastClient interface parses the returned JSON data, converting it from a columnar format to a structured object in memory.
- The TargetService inserts the astronomical target into the targets table, and returns the target data to the TargetController.
- The TargetService calls the search_tess_timeseries() method of the IMastClient interface, providing the right ascension and declination of the target, as well as a search radius of 0.005 degrees of the target.
- The MastClient implementation of the IMastClient interface sends a POST request to the MAST server at the URL https://mast.stsci.edu/api/v0/invoke, with the service field set to Mast.Caom.Filtered.Position, the ra field set to the right ascension of the target, the dec field set to the declination of the target, and the level field set to 3.
- The MAST server returns a list of observations that meet the criteria of the request (i.e., that occurred at the location of the target, and that are TESS timeseries observations at calibration level 3).
- The TargetService saves the list of observations to the tess_observations table, using the ON CONFLICT clause to ensure that it only inserts the observations if there is no record of them already in the table.
- The TargetController returns a 200 OK response to the frontend.
- The frontend displays the results of the search to the user.

*Alternative Flow A --- Target Already Cached*:

At step 6, if the service finds a cached record in the targets table, it skips steps 7-10 to resolve the MAST name and returns the target with its observations. This provides sub-200ms response time for previously resolved targets.

*Alternative Flow B --- No TESS Observations Found*:

At step 14, if the MAST server does not return any TESS observations for the target, then the target is still cached in the database (it may have other observations from other missions, or it may be observed during future sectors by TESS). The response from the server indicates that there are no TESS observations available for that target.

*Alternative Flow C --- Target Name Not Recognised*:

At step 9, if the result of the query to MAST is an empty result set, the service will return an error to the controller. The controller will return a 404 response code with the error code of TARGET_NOT_FOUND and a message asking the user to check the spelling of the target name or to attempt to use an alternative name for that object.

*Exception Flow --- MAST API Unavailable*:

If the MAST API is unreachable or returns a non-200 status code during any of the steps that involve a MAST API call, the service will log the error message with the correlation id and the MAST API response, and return an error response to the calling system. The controller will return a 502 Bad Gateway response to the client indicating that the external data source is unavailable temporarily.

*Postconditions*: The target is stored in the database with its celestial coordinates. Any available TESS observations are stored in the `tess_observations` table linked to the target.

*Related functional requirements*: FR2.1, FR2.2, FR2.3, FR2.4.

=== UC3: Fetch Light Curve

This use case describes the process of retrieving and downloading a TESS light curve for a specific observation. This includes the steps of discovering the FITS products for an observation, downloading the binary file containing the light curve data, parsing the FITS file, and performing a bulk insert of the TESS light curve data into the database.

*Actors*: Authenticated user.

*Preconditions*: User is authenticated. Target has been resolved (UC2) and there is at least one observation in TESS for this target. MAST API is accessible.

*Main Flow*:

- The user sees the list of observations for the resolved target (from UC2).
- The user chooses an observation, triggering a fetch of its light curve.
- The frontend sends a POST request to the back-end server to the following endpoint: /api/v1/targets/observations/{id}/light-curve.
- The backend validates the user’s authentication and CSRF tokens.
- The TargetController calls the TargetService::fetch_light_curve() method.
- The method first checks whether there are any light curve points already associated with the observation. If so, those points are deleted.
- The method calls the IMastClient::get_data_products() method of the IMastClient implementation, passing in the observation’s obsid.
- The MastClient implementation sends a POST request to MAST with the service field set to Mast.Caom.Products.
- MAST returns the list of data products for that observation, including the type of each product, the name of the file, the size of the file, and the data URI for the file.
- The method filters the list of data products to find the light curve file (whose filename ends in \_lc.fits). The product is found.
- The method calls the IMastClient::download_fits() method, providing the data URI of the found light curve file.
- The MastClient implementation sends an HTTP GET request to the light curve file on MAST’s server: https://mast.stsci.edu/api/v0.1/Download/file?uri=<dataURI>. The implementation receives the file content as a byte vector.
- The service calls the IFitsParser::parse_light_curve() function of the IFitsParser implementation, providing the byte vector of the light curve file’s data.
- The FitsParser implementation examines the headers of the file to locate the binary table data (HDU 2). The method inspects the TFIELDS, TTYPE, and TFORM fields of each of the columns to determine the size and endianness of each field’s data.
- The method reads the data of the TIME, PDCSAP_FLUX, SAP_FLUX, and QUALITY fields into a vector of LightCurvePoint structs. The PDCSAP_FLUX field is filtered to remove any points with NaN flux values.
- The method calls the repository pattern: the service calls the newly created ILightCurvePointRepository::save_batch() method, passing in the vector of points to be saved.
- The repository creates an SQL INSERT statement that includes all of the point data in a parameterised form and executes the statement against the database.
- The method returns the number of points to the target service.
- The target service returns a 200 OK response to the client that requested the points.
- The frontend displays a success message to the user, and offers to navigate to the light curve viewer.

*Alternative Flow A --- Light Curve Already Fetched*:

At step 6, if there are existing points in the light curve, they will be deleted before being re-ingested into the database. This ensures that if the light curve is re-fetched, the result will be the same as if it had just been fetched for the first time. Thus, it is not necessary to warn the user of the deletion of the existing data, since the data to be inserted is of the same source as the existing data and will be of equal or improved quality.

*Alternative Flow B --- No Light Curve Product Found*:

At step 10, if no data product with a filename ending in \_lc.fits is found, the service will return an error. The controller will return a 404 response code with the error code LIGHT_CURVE_NOT_FOUND. This error can occur for observations that only contain FFI data or for observations for which the SPOC pipeline has not yet generated the light curve file.

*Exception Flow --- FITS Parse Failure*:

At step 14, if the FITS file is malformed or does not contain the expected columns, an error is returned by the parser. The error is logged by the service using the correlation ID and the obsid of the observation. The controller will return a 500 internal server error (with a generic message to the client as defined by NFR7) while logging the error message in the server logs.

*Postconditions*: The table light_curve_points contains the time series data for this observation. The data is ready for visualisation via FR5.1.

*Related functional requirements*: FR2.5, FR2.6, FR2.7, FR6.1, FR6.3.

=== UC4: Record Ground-Based Observation

This use case describes the process of creating an observation session, associating equipment and location, and uploading observation images with metadata extraction.

*Actors*: Authenticated user.

*Preconditions*: The user has been authenticated. The user has created at least one observing location (FR4.1), and possibly has equipment records (FR3.1--FR3.4). The user has resolved a target (UC2).

*Main Flow*:

- The user navigates to the Observation Management page.
- The user clicks on the “New Observation Session” button and fills out the form with the following fields: selects a target to observe from the list of resolved targets, selects an observing location, and optionally adds information about the telescope, camera, mount, and filter that will be used for observation.
- The form data is sent to the backend through a POST request to the /api/v1/observation-sessions endpoint.
- The backend validates that the observation session parameters are valid (for instance, that the target, location, and equipment belong to the currently logged in user). If all parameters are valid, the backend creates the observation session in the observation_sessions database table.
- The backend responds to the user with a 201 Created response that contains the id and details of the newly-created observation session.
- The user navigates to the detail page for the newly created observation session.
- Within the session detail page, the user clicks on the “Upload Image” button.
- The user selects one or more image files of type JPEG, PNG, DNG, or FITS from their local machine.
- The files are uploaded to the server through a multipart POST request to the /api/v1/observation-sessions/{id}/images endpoint.
- The backend validates that the uploaded files are of a valid type and size. Valid files are stored on the server’s filesystem with a unique filename.
- For JPEG files, the backend extracts metadata regarding the image that was taken (such as the camera model, exposure time, ISO, the date and time when the image was taken, the focal length of the lens that was used to take the image, and the geographical coordinates of the location of the image’s subject (if visible in the image)).
- A record is created in the observation_images table for the newly uploaded image, with fields for the file path on the server, the original filename, the type of file (MIME type), the size of the file, and the extracted image metadata (stored as a JSON object in the database).
- The backend responds to the user’s request with a 201 Created response that contains the details about the newly-inserted image record (including the extracted image metadata).
- The observation session detail page displays the uploaded image along with its extracted image metadata.

*Alternative Flow A --- No Equipment Selected*:

If the user does not select any equipment items during step 2, the session that is created will have null foreign keys for the fields related to the equipment. The inclusion of equipment for a session is not required for the session to be created, as the user might not yet own that equipment, or might be observing a session that does not include any equipment use.

*Alternative Flow B --- PNG Without EXIF*:

Step 11: If the uploaded file is a PNG, there will be no results from the EXIF data extraction (PNG files do not have a standardized EXIF container). PNG files store the image with null metadata. The observation parameters can be manually entered by the user in this case.

*Exception Flow --- File Too Large*:

At step 10, if the size of the uploaded file exceeds the maximum allowed size, the request is rejected by the server with a 413 Payload Too Large response. The maximum file size is specified via an environment variable and has a default value of 50 MB.

*Postconditions*: There is an observation session in the database with the target, location, and equipment information. The images from those observations are on the filesystem with metadata in the database. The ground-based data from these observations can be compared with the TESS light curves (UC6).

=== UC5: Run Aperture Photometry

This use case describes the process of extracting a flux measurement from an observation image. The raw image can be transformed into a flux measurement that is comparable to TESS measurements.

*Actors*: Authenticated user.

*Preconditions*: The user is authenticated. An observation session exists with at least one uploaded image (UC4). The image is in a suitable format for photometry (DNG or FITS file formats; JPEG images have non-linear pixel values that make them unsuitable for photometry).

*Main Flow*:

- The user navigates to the observation page and selects the uploaded image.
- The image is displayed to the user, who can click on the target star to select it.
- Upon clicking the target star, the frontend records its (x, y) pixel coordinates.
- The frontend presents the user with the aperture parameters to configure. These include the aperture radius, the inner annulus radius, and the outer annulus radius. The values suggest default settings for the aperture.
- The frontend sends a POST request to the photometry API endpoint along with the image ID, the target star’s coordinates, and the aperture settings.
- The backend loads the image using the image loader. If the image is of type DNG, the DNG decoder (FR7.2) extracts the raw pixels from the file. If the image is of type FITS, the FITS parser extracts the image from the first HDU of the file.
- The photometry algorithm detects the centre of the source within the image close to the clicked star. This is performed by calculating the intensity-weighted mean within a search box around the target star.
- The algorithm sums the flux from the pixels within the aperture centred on the detected source.
- The algorithm calculates the background level as the median of the pixels within the annulus between the inner and outer annulus radii.
- The background contribution to the flux is subtracted from the summed flux to determine the net flux from the target.
- The flux error is calculated as the quadrature sum of three components: the square root of the summed flux (photon noise), the standard deviation of the pixels within the aperture (background noise), and the read noise of the camera’s sensor (if available).
- The backend stores the photometry results (flux, flux error, aperture settings, and centroid) into the observation_images record in the database.
- The API controller returns a 200 OK response to the frontend with the photometry results.
- The frontend displays the photometry results (along with an image overlay to indicate the aperture and annulus) to the user.

*Alternative Flow A --- Centroid Failure*:

If the centroiding algorithm cannot converge to a valid position (for instance, if no astronomical source is found close to the coordinates that the user has clicked upon), then the backend will return an error message to the client application. The user will be prompted to click on the target star again, or to change the size of the search box.

*Alternative Flow B --- JPEG Image Warning*:

At step 6, if the image is a JPEG, the backend will perform the photometry but will also return a warning that the measurements may not be reliable due to the non-linear nature of the pixel values that are applied during JPEG compression. This warning will be returned in the response metadata for the user to consider when deciding how to use the measurements.

*Postconditions*: The observation image record is updated with the measured flux, flux error, aperture parameters, and centroid position. The flux measurement can be used to overlay on TESS light curves (UC6).

=== UC6: Compare Space-Based and Ground-Based Data

UC6 describes how the user wishes to view the TESS light curve and the recorded ground-based observation on an interactive chart. Use case #6 depends upon the completion of UC3 (TESS light curve fetched) and UC4/UC5 (ground-based observation recorded and measured).

*Actors*: Authenticated user.

*Preconditions*: The user has been authenticated. The target has been resolved with at least one observation from TESS whose light curve has been fetched (UC3). The user has at least one observation session for the same target with photometry measurements (UC5).

*Main Flow*:

- User navigates to the detail page for a target that has both TESS and ground-based data
- User selects the TESS observation to display
- Frontend requests the light curve data for the selected observation from the /api/v1/targets/observations/{id}/light-curve endpoint and plots the data as a scatter plot with the x-axis representing the BTJD and the y-axis representing the PDCSAP flux
- User toggles the switch to show ground-based data on top of the light curve
- Frontend sends a request to the backend to retrieve the user’s observation sessions for the target
- For each observation session, the frontend retrieves the timestamps of the observations (converted from UTC to Barycentric Julian Date) and plots each session’s photometric measurements
- The flux of the TESS data is scaled to 1 by dividing by the median out-of-transit flux measurements. The relative flux measurements of the ground-based observations are also scaled to 1 (to their baseline). The relative flux is represented as a value between 0 and 1.
- Each of the user’s ground-based observations are plotted onto the TESS light curve with a different colour and marker style. Error bars are plotted for each observation if the ground-based flux error is known for that observation.
- User can zoom into the region of the light curve that represents the user’s observation session
- User can toggle between the TESS PDCSAP and SAP flux values
- User can hover over data points to see tooltips with relevant information about that data point

*Alternative Flow A --- No Ground-Based Data*:

If the user does not have any observation sessions or photometric measurements for this target, the toggle to display the overlay will be disabled. The user can still view the TESS light curve without the overlay.

*Alternative Flow B --- Time Mismatch*:

If the timestamps from the ground-based observations do not overlap with the TESS observation time range (e.g., the ground-based observations were taken during a different TESS sector or between TESS sectors), then the overlay will be displayed, but the data points will be plotted at different positions along the x-axis. In this case, the user will be made aware that these observations were taken at different times.

*Postconditions*: No data is modified. The use case is only a read and a visualisation process.

=== UC7: Google OAuth Login

This use case describes the authentication flow to be followed by users who want to log in to the application using their Google account.

*Actors*: Unauthenticated visitor, Google OAuth provider.

*Preconditions*: The visitor has a Google account. Backend is configured with valid Google OAuth2 client credentials. Google's authentication servers are accessible.

*Main Flow*:

- The visitor navigates to the application and clicks on the “Sign in with Google” button.
- The frontend application is configured to redirect the visitor to Google’s OAuth2 screen with the application’s client ID, redirect URI and scopes @hardt2012.
- The visitor authenticates on Google and grants the requested permissions to the application.
- Google redirects the visitor back to the Nyx application with an authorisation code included in the URL.
- The frontend application exchanges the authorisation code for authentication and refresh tokens by sending a request to Nyx’s /api/v1/auth/google API endpoint.
- Nyx’s authentication API endpoint sends a request to Google’s token endpoint with the client ID, client secret, authorisation code and redirect URI.
- Google returns an ID token, an access token and a refresh token.
- Nyx decodes the ID token to extract the user’s email address and name. Because the ID token was issued by Google’s servers over the HTTPS protocol, the authenticity of the token is guaranteed by the transport layer @openid2014.
- Nyx checks whether an account with the user’s email address already exists in the database.
- If no account exists with that email address, Nyx creates a new user account with a user ID that is set to the subject of the Google’s ID token, the user’s email address with the “verified” attribute set to true (as Google has verified the user’s email address), and sets the application’s authentication provider attribute to “google”.
- If an account exists with that user’s email address but whose authentication provider is of type “local”, Nyx sets the account’s user ID to the subject of Google’s ID token, and sets the account’s authentication provider attribute to “google”. The user can now login with either authentication provider.
- If an account with the user’s email address and authentication provider attribute of “google” already exists, Nyx does not need to make any changes to the account.
- Nyx generates an access token and a refresh token for the authenticated user following the same strategy as local authentication (FR1.7).
- Nyx returns the access and CSRF tokens in the response message, and sets the refresh token as an HttpOnly cookie.
- The user is now authenticated and can access any end points that are protected by authentication.

*Alternative Flow A --- User Denies Consent*:

At step 3, if the visitor denies the permission request on the consent screen of Google, the Google server will redirect the visitor with an error parameter. The frontend will display the message: Google login was cancelled.

*Alternative Flow B --- Invalid Authorisation Code*:

At step 6, if the authorisation code is invalid or expired, Google's token endpoint returns an error and the backend returns a 401 Unauthorized.

*Postconditions*: The user is authenticated with a valid access token and refresh token. If this was the first Google login, a new user account exists in the database linked to the Google subject ID.

*Related functional requirements*: FR1.3, FR1.7.


== MoSCoW Prioritisation <moscow>

MoSCoW prioritisation groups requirements into four categories: those that are a Must Have (the omission of which would mean that there is no minimum product), those that are a Should Have (important requirements that can be worked around), those that are a Could Have (desirable features) and those that are a Won’t Have (requirements that will be excluded from the current development iteration but may be included in future iterations). This prioritisation of requirements helps to determine the development iteration in which they will be met.

=== Must Have

The Must Have requirements form the minimum viable product for the platform. Without these requirements, the platform will not be able to fulfill its purpose of allowing users to ingest and interact with the TESS data.

- FR1.1 (User registration): Users cannot access the system without an account.
- FR1.2 (User login): Authentication is required for all data operations.
- FR1.4 (Email verification): Prevents the creation of spam accounts; only users with reachable email addresses can join the system.
- FR1.5 (Unverified user restriction): Enforces the email verification policy.
- FR1.7 (Token rotation): A security mechanism for logging in and out of the application.
- FR1.8 (Logout): Users expect the ability to log out of the application.
- FR2.1 (Target name resolution): A function that allows users to access data from TESS.
- FR2.2 (Coordinate caching): Avoids calling the MAST API; lookup times are under 200ms.
- FR2.3 (TESS observation retrieval): Retrieves information about TESS observations of a target star.
- FR2.4 (TESS observation listing): A method that allows users to view the observations of a target star.
- FR2.5 (FITS product discovery): Finds the light curve of the target star in FITS format.
- FR2.6 (FITS download and parse): Downloads and parses the light curve of a target star in FITS format.
- FR2.7 (Light curve storage): Stores the light curve for ease of access.
- FR3.1 to FR3.4 (Equipment CRUD): Equipment information is required to associate observations with a piece of equipment.
- FR4.1 (Observing location CRUD): Location information is required for time zone conversions.
- FR4.2 (Observation session creation): Used by organisations to create an observation session.
- NFR1 (API response time < 200ms): A quick response time is required to enhance the user experience.
- NFR2 (Argon2id password hashing): Argon2id passwords are required to meet security standards @owasp_password2024.
- NFR3 (Token expiry): Limits the time during which a user is logged in.
- NFR6 (Consistent response envelope): Frontend applications require a consistent response envelope to appropriately handle errors.
- NFR7 (Error information hiding): Errors exposed to the users should not expose information to potential attackers.
- NFR8 (Correlation ID tracing): Required for debugging issues with the system, especially since the system makes external API calls.
- NFR9 (Concurrent access safety): System should be able to handle multiple users of the application.
- NFR10 (Linux compatibility): The application will be deployed onto a Linux server.

=== Should Have

The Should Have requirements for a project are important, but not necessary. These requirements should be developed after the Must Have requirements are implemented.

- FR1.3 (Google OAuth2 login): Reduces registration friction, important for adoption but local login suffices.
- FR1.6 (Resend verification email): Handles edge cases well but not needed.
- FR4.3 (Image upload): Needed for ground-based telescope data but not needed for TESS data.
- FR4.4 (EXIF metadata extraction): Automates metadata capture from images but not needed as users can manually add metadata.
- FR5.1 (Interactive light curve charts): Main visualisation of data, hence needed.
- FR5.2 (Zoom and pan): Essential for light curve exploration but static chart is a fallback.
- FR5.4 (Target catalogue): Enhances navigation but users can search for targets.
- FR6.1 (Asynchronous ingestion): Prevents timeouts when processing large amounts of data but synchronous processing is fine for smaller datasets.
- FR6.2 (SSE progress reporting): Enhances user experience during long operations but polling for progress is a fallback.
- FR6.3 (Idempotent re-ingestion): Important for correctness but can be ignored during early stages of development.
- NFR4 (CSRF protection): Important security measure in addition to OAuth2 authentication.
- NFR5 (Rate limiting): Helps protect against brute-force attacks but less important for a single-user application.
- NFR11 (RAII for FITS files): Prevents memory leaks when files are no longer needed but less important than other features.
- NFR12 (Batch size tuning): Optimises the application for fast ingestion of data but the default setting works correctly albeit slowly.

=== Could Have

Requirements that could help add significant value to the platform are included in the Could Have section. These features are beyond the scope of the must have and should have requirements but may be implemented if time permits after the fulfillment of those requirements.

- FR5.3: Describes the value proposition, which is based on the assumption that photometry is functional.
- FR7.1: Adds support for photometry, though this is not essential for the system’s operation.
- FR7.2: Allows for photometry of raw images, in addition to the alternative input of FITS images.
- FR7.3: Performs aperture photometry on the input images; this will require some implementation effort.
- FR7.4: Estimates the error in the photometry measurements; this feature will add scientific rigour to the application, but can be added incrementally.

=== Won't Have (This Iteration)

The following requirements are not being met in this iteration of the project. Though these requirements will be valuable features for the software product, implementing these features will exceed the scope of this project.

- Real-time telescope control is outside the scope of this project. Although protocols like ASCOM and INDI exist for communicating with telescope hardware, their integration would be outside the scope of this project.
- Automated plate solving uses large reference catalogues of stars and algorithms to determine which part of the sky an exposure shows. Since users input the name of their target of observation, solving for the plate is outside the scope of this project.
- A differential photometry pipeline is a project in itself. The application includes a simple aperture photometry package that calculates the brightness of objects in observations. A full photometry pipeline could be implemented in future versions of this project.
- An observation planner that calculates the visibility of celestial objects from a given location on the Earth is a valuable feature for planning observations. This feature is implemented in the frontend only and does not relate to the data processing pipeline.
- While features like sharing observations with other users or viewing community galleries of observations is beneficial to the concept of astronomy as a social science, these features are outside of the scope of this project because they are not needed for the single-user observation workflow.
- This project is specifically designed to run as a single instance. Distributed systems concepts such as load balancing or container orchestration are outside the scope of this project. The database uses features like MVCC that allow it to handle concurrent user sessions. Drogon, the web framework used to implement the backend API, is also multi-threaded which allows it to handle multiple requests from users simultaneously.

The MoSCoW categorisation reflects the project's priorities as a data engineering dissertation. The Must Have requirements focus on the data pipeline (target resolution, observation retrieval, FITS parsing, light curve storage) and the security baseline (authentication, password hashing, error handling). The Should Have requirements extend the pipeline with asynchronous execution and progress reporting, add the frontend visualisation, and implement additional security measures. The Could Have requirements address the ground-based photometry workflow, which is the most technically demanding aspect of the platform but is dependent on the core pipeline being functional first. The Won't Have requirements are documented to set clear expectations about the project's boundaries.


== Requirements Traceability <traceability_section>

Requirements traceability ensures that each objective of the project is both satisfied by at least one requirement, and that each requirement is contributing to at least one of the project’s objectives. This section provides two traceability views: one mapping project objectives (from @introduction) to functional requirements, and one mapping functional requirements to the non-functional requirements that constrain their implementation.

=== Objectives to Functional Requirements

The following map each project objective to the functional requirements that implement it. The objectives are numbered as they appear in the Aim and Objectives section of @introduction.

- Objective 1: Develop the C++ backend (REST API, background jobs, SSE) - FR1.1-FR1.8, FR2.1-FR2.7, FR3.1-FR3.4, FR4.1-FR4.4, FR6.1-FR6.3.
- Objective 2: Build the data ingestion pipeline (MAST API, TESS, FITS) - FR2.1, FR2.2, FR2.3, FR2.5, FR2.6, FR2.7, FR6.1, FR6.3.
- Objective 3: Implement PostgreSQL persistence layer - FR2.2, FR2.3, FR2.7, FR3.1-FR3.4, FR4.1, FR4.2.
- Objective 4: Develop user authentication - FR1.1, FR1.2, FR1.3, FR1.4, FR1.5, FR1.6, FR1.7, FR1.8.
- Objective 5: Implement observation management functionality - FR3.1-FR3.4, FR4.1-FR4.4, FR7.1-FR7.4.
- Objective 6: Build the React frontend - FR5.1, FR5.2, FR5.3, FR5.4.
- Objective 7: Validate system with real TESS data - FR2.6, FR2.7, FR5.1, FR5.3.

Each objective has at least two functional requirements associated with it. Objective 1 (REST API backend) has the most requirements mapped to it overall, as it forms the basis of the other components of the system. Objective 2 (data ingestion pipeline) is linked to the target management and data ingestion pipeline requirements. Finally, objective 7 (validation) is associated with the FITS parsing and visualisation requirements, as the validation component requires both the ability to ingest real data and display it.

=== Functional Requirements to Non-Functional Requirements

The following maps the functional requirements to the non-functional requirements that constrain and quality gate the implementation of the functional requirements. This mapping ensures that implementers understand which non-functional requirements apply to each functional requirement.

- FR1.1 (Registration): NFR2 (Argon2id hashing), NFR5 (rate limiting), NFR6 (response envelope), NFR7 (error hiding), NFR8 (correlation ID).
- FR1.2 (Login): NFR2 (password verification), NFR3 (token expiry), NFR4 (CSRF token issued), NFR5 (rate limiting), NFR6, NFR7, NFR8.
- FR1.3 (Google OAuth): NFR3 (token expiry), NFR4 (CSRF token issued), NFR6, NFR7, NFR8.
- FR1.7 (Token rotation): NFR3 (token expiry enforcement), NFR4 (CSRF rotation), NFR9 (concurrent refresh safety).
- FR2.1 to FR2.3 (Target resolution, observation retrieval): NFR1 (200ms for cached targets), NFR6, NFR7, NFR8, NFR9 (ON CONFLICT for concurrent resolution).
- FR2.5 to FR2.7 (FITS discovery, download, parse, store): NFR1 (200ms for stored light curves), NFR6, NFR7, NFR8, NFR11 (RAII for FITS data), NFR12 (batch insert tuning).
- FR3.1 to FR3.4 (Equipment CRUD): NFR1 (200ms response), NFR4 (CSRF on mutations), NFR6, NFR7, NFR8, NFR9.
- FR4.1 to FR4.4 (Observation management): NFR1, NFR4, NFR6, NFR7, NFR8, NFR9.
- FR5.1 to FR5.4 (Visualisation): NFR1 (200ms for data retrieval), NFR10 (browser compatibility).
- FR6.1 to FR6.3 (Pipeline): NFR8 (correlation ID across async boundaries), NFR9 (idempotent writes), NFR11, NFR12.
- FR7.1 to FR7.4 (Image processing): NFR6, NFR7, NFR8, NFR11 (RAII for image buffers).

The traceability mappings reveal several patterns. First, NFR6, NFR7, and NFR8 are satisfied by every functional requirement - these are implemented in the middleware. Second, NFR9 is satisfied by functional requirements involving concurrent writes to the same resources, such as when two users attempt to resolve the same target, or when a user modifies equipment while viewing it in another tab in the application, or when attempting to ingest the same pipeline twice in succession. Finally, NFR11 is satisfied only by the requirements related to FITS files and image processing - both of these involve large amounts of binary data to be processed.

=== Summary

Having performed each of the steps described in this chapter, the Nyx platform has requirements that have been established through the analysis of stakeholders, functional requirements, non-functional requirements, data requirements, use cases, and the MoSCoW and traceability matrices. The following chapter will translate these requirements into a system design for the Nyx platform.
