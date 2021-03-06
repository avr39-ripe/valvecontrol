#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "../include/configuration.h"

bool serverStarted = false;
HttpServer server;
extern String StrT, StrRH, StrTime; // Sensors string values
extern int counter;
extern float curr_temp;
extern void process(void);

void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars = tmpl->variables();
	vars["Time"] = String(counter);
	vars["Temp"] = String(curr_temp);
	response.sendTemplate(tmpl);
}

void onConfiguration(HttpRequest &request, HttpResponse &response)
{
//	ValveConfig cfg = loadConfig();
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		debugf("Update config");
		// Update config
		if (request.getPostParameter("SSID").length() > 0) // Network
		{
			ActiveConfig.NetworkSSID = request.getPostParameter("SSID");
			ActiveConfig.NetworkPassword = request.getPostParameter("Password");
		}
		if (request.getPostParameter("set_temp").length() > 0) // Settings
		{
			ActiveConfig.set_temp = request.getPostParameter("set_temp").toFloat();
			ActiveConfig.temp_delta = request.getPostParameter("temp_delta").toFloat();
			ActiveConfig.temp_interval = request.getPostParameter("temp_interval").toInt();
			ActiveConfig.switch_interval = request.getPostParameter("switch_interval").toInt();

			DynamicJsonBuffer jsonBuffer;
			JsonObject& root = jsonBuffer.createObject();

			JsonObject& settings = jsonBuffer.createObject();
			root["settings"] = settings;
			settings["set_temp"] = ActiveConfig.set_temp;
			settings["temp_delta"] = ActiveConfig.temp_delta;
			settings["temp_interval"] = ActiveConfig.temp_interval;
			settings["switch_interval"] = ActiveConfig.switch_interval;

			root.printTo(Serial);
			Serial.println();
		}

		saveConfig(ActiveConfig);
		response.redirect();
	}

	debugf("Send template");
	TemplateFileStream *tmpl = new TemplateFileStream("config.html");
	auto &vars = tmpl->variables();
	vars["SSID"] = ActiveConfig.NetworkSSID;
	vars["set_temp"] = String(ActiveConfig.set_temp, 2);
	vars["temp_delta"] = String(ActiveConfig.temp_delta, 2);
	vars["temp_interval"] = String((int)ActiveConfig.temp_interval);
	vars["switch_interval"] = String((int)ActiveConfig.switch_interval);
	response.sendTemplate(tmpl);
}

void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void onAJAXGetTemp(HttpRequest &request, HttpResponse &response)
{
	Serial.println("{\"command\":\"get_temp\"}");
	process();

	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();
	JsonObject& temperature = json.createNestedObject("temperature");
	json["temperature"] =  temperature;
	temperature["curr_temp"] = curr_temp;
	json["counter"] = counter;

	response.sendJsonObject(stream);
}

void onAJAXGetArdConf(HttpRequest &request, HttpResponse &response)
{
	Serial.println("{\"command\":\"get_conf\"}");

	char buf[255];
	int length = Serial.readBytesUntil('\r', buf, 255);
	if ( length > 0)
	{
		buf[length]='\0';
		response.sendString(buf);
	}
}

void startWebServer()
{
	if (serverStarted) return;

	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/config", onConfiguration);
	server.addPath("/temp", onAJAXGetTemp);
	server.addPath("/ard_conf", onAJAXGetArdConf);
	server.setDefaultHandler(onFile);
	serverStarted = true;

	if (WifiStation.isEnabled())
		debugf("STA: %s", WifiStation.getIP().toString().c_str());
	if (WifiAccessPoint.isEnabled())
		debugf("AP: %s", WifiAccessPoint.getIP().toString().c_str());
}

/// FileSystem Initialization ///

//Timer downloadTimer;
//HttpClient downloadClient;
//int dowfid = 0;
//void downloadContentFiles()
//{
//	if (!downloadTimer.isStarted())
//	{
//		downloadTimer.initializeMs(3000, downloadContentFiles).start();
//	}
//
//	if (downloadClient.isProcessing()) return; // Please, wait.
//	debugf("DownloadContentFiles");
//
//	if (downloadClient.isSuccessful())
//		dowfid++; // Success. Go to next file!
//	downloadClient.reset(); // Reset current download status
//
//	if (dowfid == 0)
//		downloadClient.downloadFile("http://avr.zz.mu/templates/ValveControl/ValveControlTime.html", "index.html");
//	else if (dowfid == 1)
//		downloadClient.downloadFile("http://avr.zz.mu/templates/ValveControl/ValveConfig.html", "config.html");
//	else if (dowfid == 2)
//		downloadClient.downloadFile("http://avr.zz.mu/templates/ValveControl/ValveAPI.html", "api.html");
//	else if (dowfid == 3)
//		downloadClient.downloadFile("http://avr.zz.mu/templates/bootstrap.css.gz");
//	else if (dowfid == 4)
//		downloadClient.downloadFile("http://avr.zz.mu/templates/jquery.js.gz");
//	else
//	{
//		// Content download was completed
//		downloadTimer.stop();
//		startWebServer();
//	}
//}
