#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "esp32_secret.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"

extern int LED;
extern String WiFiAddr;
bool isAutoMode; // Variable to determine automatic mode on ESP32 CAM

extern float latitude;	// Variable to store latitude
extern float longitude; // Variable to store longitude

// Define structure for running average filter
typedef struct
{
	size_t size;
	size_t index;
	size_t count;
	int sum;
	int *values;
} ra_filter_t;

// Structure for transmitting JPEG images in chunks
typedef struct
{
	httpd_req_t *req;
	size_t len;
} jpg_chunking_t;

// Define constants for image data transmission
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

// Function to initialize the filter
static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size)
{
	memset(filter, 0, sizeof(ra_filter_t));
	filter->values = (int *)malloc(sample_size * sizeof(int));
	if (!filter->values)
	{
		return NULL;
	}
	memset(filter->values, 0, sample_size * sizeof(int));
	filter->size = sample_size;
	return filter;
}

// Function to update and calculate the average value of the filter
static int ra_filter_run(ra_filter_t *filter, int value)
{
	if (!filter->values)
	{
		return value;
	}
	filter->sum -= filter->values[filter->index];
	filter->values[filter->index] = value;
	filter->sum += filter->values[filter->index];
	filter->index++;
	filter->index = filter->index % filter->size;
	if (filter->count < filter->size)
	{
		filter->count++;
	}
	return filter->sum / filter->count;
}

// Function to support encoding and sending JPEG images in chunks
static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
	jpg_chunking_t *j = (jpg_chunking_t *)arg;
	if (!index)
	{
		j->len = 0;
	}
	if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
	{
		return 0;
	}
	j->len += len;
	return len;
}

// Function to set CORS headers
static void set_cors_headers(httpd_req_t *req)
{
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
	httpd_resp_set_hdr(req, "Referrer-Policy", "no-referrer");
}

// Handler for OPTIONS method (CORS)
static esp_err_t options_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	return httpd_resp_send(req, NULL, 0); // No content needed
}

// Handler for streaming image data
static esp_err_t stream_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	camera_fb_t *fb = NULL;
	esp_err_t res = ESP_OK;
	size_t _jpg_buf_len = 0;
	uint8_t *_jpg_buf = NULL;
	char *part_buf[64];

	static int64_t last_frame = 0;
	if (!last_frame)
	{
		last_frame = esp_timer_get_time();
	}

	res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
	if (res != ESP_OK)
	{
		return res;
	}

	while (true)
	{
		fb = esp_camera_fb_get(); // Get image from camera
		if (!fb)
		{
			Serial.printf("Camera capture failed");
			res = ESP_FAIL;
		}
		else
		{
			if (fb->format != PIXFORMAT_JPEG)
			{
				bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
				esp_camera_fb_return(fb);
				fb = NULL;
				if (!jpeg_converted)
				{
					Serial.printf("JPEG compression failed");
					res = ESP_FAIL;
				}
			}
			else
			{
				_jpg_buf_len = fb->len;
				_jpg_buf = fb->buf;
			}
		}
		if (res == ESP_OK)
		{
			size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
			res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
		}
		if (res == ESP_OK)
		{
			res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
		}
		if (res == ESP_OK)
		{
			res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
		}
		if (fb)
		{
			esp_camera_fb_return(fb);
			fb = NULL;
			_jpg_buf = NULL;
		}
		else if (_jpg_buf)
		{
			free(_jpg_buf);
			_jpg_buf = NULL;
		}
		if (res != ESP_OK)
		{
			break;
		}
		int64_t fr_end = esp_timer_get_time();

		int64_t frame_time = fr_end - last_frame;
		last_frame = fr_end;
		frame_time /= 1000;
		uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
	}

	last_frame = 0;
	return res;
}

// Handler for controlling camera parameters via URL
static esp_err_t cmd_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	char *buf;
	size_t buf_len;
	char variable[32] = {
		0,
	};
	char value[32] = {
		0,
	};
	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1)
	{
		buf = (char *)malloc(buf_len);
		if (!buf)
		{
			httpd_resp_send_500(req);
			return ESP_FAIL;
		}
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK && httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
			{
			}
			else
			{
				free(buf);
				httpd_resp_send_404(req);
				return ESP_FAIL;
			}
		}
		else
		{
			free(buf);
			httpd_resp_send_404(req);
			return ESP_FAIL;
		}
		free(buf);
	}
	else
	{
		httpd_resp_send_404(req);
		return ESP_FAIL;
	}
	int val = atoi(value);
	sensor_t *s = esp_camera_sensor_get();
	int res = 0;

	if (!strcmp(variable, "framesize"))
	{
		if (s->pixformat == PIXFORMAT_JPEG)
			res = s->set_framesize(s, (framesize_t)val);
	}
	else if (!strcmp(variable, "quality"))
		res = s->set_quality(s, val);
	else if (!strcmp(variable, "contrast"))
		res = s->set_contrast(s, val);
	else if (!strcmp(variable, "brightness"))
		res = s->set_brightness(s, val);
	else if (!strcmp(variable, "saturation"))
		res = s->set_saturation(s, val);
	else if (!strcmp(variable, "gainceiling"))
		res = s->set_gainceiling(s, (gainceiling_t)val);
	else if (!strcmp(variable, "colorbar"))
		res = s->set_colorbar(s, val);
	else if (!strcmp(variable, "awb"))
		res = s->set_whitebal(s, val);
	else if (!strcmp(variable, "agc"))
		res = s->set_gain_ctrl(s, val);
	else if (!strcmp(variable, "aec"))
		res = s->set_exposure_ctrl(s, val);
	else if (!strcmp(variable, "hmirror"))
		res = s->set_hmirror(s, val);
	else if (!strcmp(variable, "vflip"))
		res = s->set_vflip(s, val);
	else if (!strcmp(variable, "awb_gain"))
		res = s->set_awb_gain(s, val);
	else if (!strcmp(variable, "agc_gain"))
		res = s->set_agc_gain(s, val);
	else if (!strcmp(variable, "aec_value"))
		res = s->set_aec_value(s, val);
	else if (!strcmp(variable, "aec2"))
		res = s->set_aec2(s, val);
	else if (!strcmp(variable, "dcw"))
		res = s->set_dcw(s, val);
	else if (!strcmp(variable, "bpc"))
		res = s->set_bpc(s, val);
	else if (!strcmp(variable, "wpc"))
		res = s->set_wpc(s, val);
	else if (!strcmp(variable, "raw_gma"))
		res = s->set_raw_gma(s, val);
	else if (!strcmp(variable, "lenc"))
		res = s->set_lenc(s, val);
	else if (!strcmp(variable, "special_effect"))
		res = s->set_special_effect(s, val);
	else if (!strcmp(variable, "wb_mode"))
		res = s->set_wb_mode(s, val);
	else if (!strcmp(variable, "ae_level"))
		res = s->set_ae_level(s, val);
	else
	{
		res = -1;
	}
	if (res)
	{
		return httpd_resp_send_500(req);
	}
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	return httpd_resp_send(req, NULL, 0);
}

// Handler to return the status of the camera and GPS coordinates in JSON format
static esp_err_t status_handler(httpd_req_t *req)
{
	Serial.println("Status handler called");
	set_cors_headers(req);
	static char json_response[1024];
	sensor_t *s = esp_camera_sensor_get();
	char *p = json_response;
	*p++ = '{';
	// ESP32 CAM status
	p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
	p += sprintf(p, "\"quality\":%u,", s->status.quality);
	p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
	p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
	p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
	p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
	p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
	p += sprintf(p, "\"awb\":%u,", s->status.awb);
	p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
	p += sprintf(p, "\"aec\":%u,", s->status.aec);
	p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
	p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
	p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
	p += sprintf(p, "\"agc\":%u,", s->status.agc);
	p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
	p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
	p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
	p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
	p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
	p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
	p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
	p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
	p += sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
	// Add latitude and longitude to the JSON response
	p += sprintf(p, "\"latitude\":%.6f,", latitude);
	p += sprintf(p, "\"longitude\":%.6f", longitude);
	*p++ = '}';
	*p++ = 0;
	httpd_resp_set_type(req, "application/json");

	return httpd_resp_send(req, json_response, strlen(json_response));
}

// Handler for the main web page
static esp_err_t index_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
	set_cors_headers(req);
	String page = "";
	page += "<!DOCTYPE html>\n";
	page += "<html>\n";
	page += "<head>\n";
	page += "<meta charset=\"UTF-8\">\n";
	page += "<title>ESP32 CAM car</title>\n";
	page += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\n";
	page += "<link href='https://api.mapbox.com/mapbox-gl-js/v2.6.1/mapbox-gl.css' rel='stylesheet' />";

	page += "<style>";
	page += "button { touch-action: manipulation; color: black; }";
	page += "</style>\n";

	page += "<script>var xhttp = new XMLHttpRequest();</script>";
	page += "<script>function getsend(arg) { xhttp.open('GET', arg +'?' + new Date().getTime(), true); xhttp.send() } </script>";
	page += "</head>\n";
	page += "<body>\n";
	page += "<p align=center><img src='data:image/png;base64,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' style='width:300px;'></p><br/><br/>"; // Add Base64 image here
	page += "<h1 align=center>ESP32 CAM car</h1>\n";
	page += "<p align=center><IMG SRC='http://" + WiFiAddr + ":81/stream' style='width:300px; transform:rotate(180deg);'></p><br/><br/>";
	page += "<p align=center> <button style=background-color:lightgrey;width:90px;height:80px onmousedown=getsend('go') onmouseup=getsend('stop') ontouchstart=getsend('go') ontouchend=getsend('stop') ><b>TIEN</b></button> </p>";
	page += "<p align=center>";
	page += "<button style=background-color:lightgrey;width:90px;height:80px; onmousedown=getsend('left') onmouseup=getsend('stop') ontouchstart=getsend('left') ontouchend=getsend('stop')><b>TRAI</b></button>&nbsp;";
	page += "<button style=background-color:red;width:90px;height:80px onclick=getsend('/tongleautomode')><b>Auto Mode</b></button>";
	page += "<button style=background-color:lightgrey;width:90px;height:80px onmousedown=getsend('right') onmouseup=getsend('stop') ontouchstart=getsend('right') ontouchend=getsend('stop')><b>PHAI</b></button>";
	page += "</p>";
	page += "<p align=center><button style=background-color:lightgrey;width:90px;height:80px onmousedown=getsend('back') onmouseup=getsend('stop') ontouchstart=getsend('back') ontouchend=getsend('stop') ><b>LUI</b></button></p>";
	page += "<p align=center>";
	page += "</p>";

	page += "<p align=center><b>Vĩ Độ:</b> <span id='latitude'>" + String(latitude, 6) + "</span> <b>Kinh Độ:</b> <span id='longitude'>" + String(longitude, 6) + "</span></p>";
	page += "<div id='map' style='width: 100%; height: 400px;'></div>";
	page += "<script src='https://atlas.microsoft.com/sdk/javascript/mapcontrol/2/atlas.min.js'></script>";
	page += "<script>";
	page += "	var map = new atlas.Map('map', {";
	page += "	center: [" + String(longitude, 6) + ", " + String(latitude, 6) + "],";
	page += "	zoom: 18,";
	page += "	authOptions: {";
	page += "		authType: 'subscriptionKey',";
	page += "		subscriptionKey: '" + String(AZURE_MAPS_API) + "'";
	page += "	},";
	page += "});";
	page += "map.events.add('ready', function () {";
	page += "    var position = new atlas.data.Position(" + String(longitude, 6) + ", " + String(latitude, 6) + ");";
	page += "    var point = new atlas.data.Point(position);";
	page += "    var marker = new atlas.HtmlMarker({ position: position });";
	page += "    map.markers.add(marker);";
	page += "});";
	page += "function updateLocation() {";
	page += "  var xhr = new XMLHttpRequest();";
	page += "  xhr.open('GET', '/status', true);";
	page += "  xhr.onreadystatechange = function() {";
	page += "    if (xhr.readyState == 4 && xhr.status == 200) {";
	page += "      var response = JSON.parse(xhr.responseText);";
	page += "      document.getElementById('latitude').innerText = response.latitude.toFixed(6);";
	page += "      document.getElementById('longitude').innerText = response.longitude.toFixed(6);";
	page += "      map.setCamera({ center: [response.longitude, response.latitude] });"; // Update map center
	page += "      var position = new atlas.data.Position(response.longitude, response.latitude);";
	page += "      var point = new atlas.data.Point(position);";
	page += "      var marker = new atlas.HtmlMarker({ position: position });";
	page += "      map.markers.clear();";	  // Clear existing markers
	page += "      map.markers.add(marker);"; // Add new marker
	page += "    }";
	page += "  };";
	page += "  xhr.send();";
	page += "}";
	page += "setInterval(updateLocation, 1000);";
	page += "</script>";

	page += "<footer style='text-align:center; padding:10px; background-color:#f1f1f1;'>"; // Add footer here
	page += "<p>Thank you for visiting!</p>";
	page += "<p>20040311 - Đinh Hoàng Duy</p>";
	page += "<p>20109941 - Lý Đức Tuấn   </p>";
	page += "</footer>";
	page += "</body>\n";
	page += "</html>";

	return httpd_resp_send(req, &page[0], strlen(&page[0]));
}

// (Other handlers for car control: go_handler, back_handler, etc.)
static esp_err_t go_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	Serial.println("/F");
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "OK", 2);
}
static esp_err_t back_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	Serial.println("/B");
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "OK", 2);
}

static esp_err_t left_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	Serial.println("/L");
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "OK", 2);
}
static esp_err_t right_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	Serial.println("/R");
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "OK", 2);
}

static esp_err_t stop_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	Serial.println("/S");
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "OK", 2);
}
// Handler to toggle automatic mode
static esp_err_t tongleautomode_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	if (isAutoMode == false)
	{
		isAutoMode = true;
		Serial.println("/AUTO");
		// digitalWrite(LED, HIGH);
	}
	else
	{
		isAutoMode = false;
		Serial.println("/MANUAL");
		// digitalWrite(LED, LOW);
	}

	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "OK", 2);
}

// Function to start the camera server
void startCameraServer()
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;

	httpd_uri_t go_uri = {
		.uri = "/go",
		.method = HTTP_GET,
		.handler = go_handler,
		.user_ctx = NULL};

	httpd_uri_t back_uri = {
		.uri = "/back",
		.method = HTTP_GET,
		.handler = back_handler,
		.user_ctx = NULL};

	httpd_uri_t stop_uri = {
		.uri = "/stop",
		.method = HTTP_GET,
		.handler = stop_handler,
		.user_ctx = NULL};

	httpd_uri_t left_uri = {
		.uri = "/left",
		.method = HTTP_GET,
		.handler = left_handler,
		.user_ctx = NULL};

	httpd_uri_t right_uri = {
		.uri = "/right",
		.method = HTTP_GET,
		.handler = right_handler,
		.user_ctx = NULL};

	httpd_uri_t tongleautomode_uri = {
		.uri = "/tongleautomode",
		.method = HTTP_GET,
		.handler = tongleautomode_handler,
		.user_ctx = NULL};

	httpd_uri_t index_uri = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = index_handler,
		.user_ctx = NULL};

	httpd_uri_t status_uri = {
		.uri = "/status*",
		.method = HTTP_GET,
		.handler = status_handler,
		.user_ctx = NULL};

	httpd_uri_t cmd_uri = {
		.uri = "/control",
		.method = HTTP_GET,
		.handler = cmd_handler,
		.user_ctx = NULL};

	httpd_uri_t stream_uri = {
		.uri = "/stream",
		.method = HTTP_GET,
		.handler = stream_handler,
		.user_ctx = NULL};

	httpd_uri_t options_uri = {
		.uri = "/*", // Apply to all URIs
		.method = HTTP_OPTIONS,
		.handler = options_handler,
		.user_ctx = NULL};

	ra_filter_init(&ra_filter, 20);
	Serial.printf("Starting web server on port: '%d'", config.server_port);
	if (httpd_start(&camera_httpd, &config) == ESP_OK)
	{
		// Register handlers for the camera server
		httpd_register_uri_handler(camera_httpd, &status_uri);
		httpd_register_uri_handler(camera_httpd, &index_uri);
		httpd_register_uri_handler(camera_httpd, &go_uri);
		httpd_register_uri_handler(camera_httpd, &back_uri);
		httpd_register_uri_handler(camera_httpd, &stop_uri);
		httpd_register_uri_handler(camera_httpd, &left_uri);
		httpd_register_uri_handler(camera_httpd, &right_uri);
		// httpd_register_uri_handler(camera_httpd, &tongleheadlight_uri);
		httpd_register_uri_handler(camera_httpd, &tongleautomode_uri);
		httpd_register_uri_handler(camera_httpd, &options_uri);
	}

	config.server_port += 1;
	config.ctrl_port += 1;
	Serial.printf("Starting stream server on port: '%d'", config.server_port);
	if (httpd_start(&stream_httpd, &config) == ESP_OK)
	{
		// Register handler for image streaming
		httpd_register_uri_handler(stream_httpd, &stream_uri);
	}
}