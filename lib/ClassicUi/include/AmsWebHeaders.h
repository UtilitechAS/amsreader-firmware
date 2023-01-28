static const char HEADER_CACHE_CONTROL[] PROGMEM = "Cache-Control";
static const char HEADER_PRAGMA[] PROGMEM = "Pragma";
static const char HEADER_EXPIRES[] PROGMEM = "Expires";
static const char HEADER_AUTHENTICATE[] PROGMEM = "WWW-Authenticate";
static const char HEADER_LOCATION[] PROGMEM = "Location";

static const char CACHE_CONTROL_NO_CACHE[] PROGMEM = "no-cache, no-store, must-revalidate";
static const char CACHE_1HR[] PROGMEM = "public, max-age=3600";
static const char PRAGMA_NO_CACHE[] PROGMEM = "no-cache";
static const char EXPIRES_OFF[] PROGMEM = "-1";
static const char AUTHENTICATE_BASIC[] PROGMEM = "Basic realm=\"Secure Area\"";

static const char MIME_PLAIN[] PROGMEM = "text/plain";
static const char MIME_HTML[] PROGMEM = "text/html";
static const char MIME_JSON[] PROGMEM = "application/json";
