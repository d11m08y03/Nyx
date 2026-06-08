// Auth
export interface TokenResponse {
  access_token: string;
  csrf_token: string;
}

export interface UserProfile {
  id: string;
  email: string;
  email_verified: boolean;
  display_name?: string;
}

// Targets — fields match backend TargetController::target_response_to_json
export interface Target {
  id: string;
  canonical_name: string;
  right_ascension?: number | null;
  declination?: number | null;
  target_type?: string | null;
  tess_observations?: TessObservation[];
}

// TESS observations — fields match TessObservationController JSON
export interface TessObservation {
  id: string;
  obsid: string;
  cadence_seconds: number;
  start_time: number;
  end_time: number;
  data_uri?: string | null;
}

// Light curve points — fields match PostgresLightCurvePointRepository JSON
export interface LightCurvePoint {
  time: number;
  pdcsap_flux?: number | null;
  pdcsap_flux_err?: number | null;
  sap_flux?: number | null;
  quality?: number;
}

export interface LightCurveComparison {
  target: {
    id: string;
    canonical_name: string;
    right_ascension?: number | null;
    declination?: number | null;
  };
  time_system: string;
  time_system_note: string;
  tess: {
    tess_observation_id: string;
    obsid: string;
    cadence_seconds: number;
    point_count: number;
    points: LightCurvePoint[];
  };
  user_observations: {
    session_count: number;
    point_count: number;
    points: UserObservationPoint[];
  };
}

export interface UserObservationPoint {
  time: number;
  relative_flux: number;
  relative_flux_error?: number | null;
  session_id: string;
  captured_at: string;
}

// Equipment — fields match their respective controller JSON
export interface Telescope {
  id: string;
  name: string;
  aperture_mm: number;
  focal_length_mm: number;
  optical_design: string;
  brand?: string | null;
  model?: string | null;
}

export interface Camera {
  id: string;
  name: string;
  sensor_type: string;
  brand?: string | null;
  model?: string | null;
  pixel_size_um?: number | null;
  resolution?: string | null;
}

export interface Mount {
  id: string;
  name: string;
  mount_type: string;
  is_tracked?: boolean | null;
  has_goto?: boolean | null;
  brand?: string | null;
  model?: string | null;
}

export interface Filter {
  id: string;
  name: string;
  band: string;
  bandwidth_nm?: number | null;
  brand?: string | null;
  model?: string | null;
}

// Locations
export interface ObservingLocation {
  id: string;
  name: string;
  latitude: number;
  longitude: number;
  bortle_class?: number | null;
}

// Observation images — fields match ObservationSessionController::image_to_json
export interface ObservationImage {
  id: string;
  session_id: string;
  original_filename: string;
  file_size_bytes: number;
  mime_type: string;
  created_at: string;
  captured_at?: string | null;
  camera_model?: string | null;
  exposure_time_seconds?: number | null;
  iso_speed?: number | null;
  image_width?: number | null;
  image_height?: number | null;
  target_x?: number | null;
  target_y?: number | null;
  raw_flux?: number | null;
  raw_flux_error?: number | null;
  relative_flux?: number | null;
  relative_flux_error?: number | null;
  photometry_status?: string | null;
  photometry_error_message?: string | null;
}

// Observation sessions — fields match ObservationSessionController::session_to_json
// NOTE: no nested target/telescope/camera/mount/location/filter objects — only IDs
export interface ObservationSession {
  id: string;
  user_id: string;
  target_id: string;
  telescope_id: string;
  camera_id: string;
  mount_id: string;
  location_id: string;
  filter_id?: string | null;
  notes?: string | null;
  created_at: string;
  updated_at: string;
  images: ObservationImage[];
}

// Photometry — fields match ObservationSessionController photometry response
export interface PhotometryStatus {
  session_id: string;
  status: string;
  message: string;
}

// Pagination
export interface Paginated<T> {
  items: T[];
  pagination: {
    limit: number;
    next_cursor?: string;
    has_more: boolean;
  };
}
