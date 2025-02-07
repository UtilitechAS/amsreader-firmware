/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

static const char HEADER_AUTHORIZATION[] PROGMEM = "Authorization";
static const char HEADER_CACHE_CONTROL[] PROGMEM = "Cache-Control";
static const char HEADER_CONTENT_ENCODING[] PROGMEM = "Content-Encoding";
static const char HEADER_PRAGMA[] PROGMEM = "Pragma";
static const char HEADER_EXPIRES[] PROGMEM = "Expires";
static const char HEADER_AUTHENTICATE[] PROGMEM = "WWW-Authenticate";
static const char HEADER_LOCATION[] PROGMEM = "Location";
static const char HEADER_ACCESS_CONTROL_ALLOW_ORIGIN[] PROGMEM = "Access-Control-Allow-Origin";
static const char HEADER_ACCESS_CONTROL_REQUEST_PRIVATE_NETWORK[] PROGMEM = "Access-Control-Request-Private-Network";
static const char HEADER_ACCESS_CONTROL_ALLOW_PRIVATE_NETWORK[] PROGMEM = "Access-Control-Allow-Private-Network";
static const char HEADER_REFERER[] PROGMEM = "Referer";
static const char HEADER_ORIGIN[] PROGMEM = "Origin";

static const char CACHE_CONTROL_NO_CACHE[] PROGMEM = "no-cache, no-store, must-revalidate";
static const char CONTENT_ENCODING_GZIP[] PROGMEM = "gzip";
static const char CACHE_1DA[] PROGMEM = "public, max-age=86400";
static const char CACHE_1MO[] PROGMEM = "public, max-age=2630000";
static const char CACHE_1YR[] PROGMEM = "public, max-age=31536000";
static const char PRAGMA_NO_CACHE[] PROGMEM = "no-cache";
static const char EXPIRES_OFF[] PROGMEM = "-1";
static const char AUTHENTICATE_BASIC[] PROGMEM = "Basic realm=\"Secure Area\"";

static const char MIME_PLAIN[] PROGMEM = "text/plain";
static const char MIME_HTML[] PROGMEM = "text/html";
static const char MIME_JSON[] PROGMEM = "application/json";
static const char MIME_CSS[] PROGMEM = "text/css";
static const char MIME_JS[] PROGMEM = "text/javascript";

static const char ORIGIN_AMSLESER_CLOUD[] PROGMEM = "https://www.amsleser.cloud";
