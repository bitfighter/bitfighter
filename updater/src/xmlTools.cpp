/*
 Copyright 2007 Don HO <don.h@free.fr>

 This file is part of GUP.

 GUP is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 GUP is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with GUP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "xmlTools.h"

using namespace std;
using namespace tinyxml2;

GupParameters::GupParameters(const char * xmlFileName)
{
	_xmlDoc.LoadFile(xmlFileName);

	XMLElement *root = _xmlDoc.FirstChildElement("GUPInput");
	if (!root)
		throw exception("It's not a valid GUP input xml.");

	XMLElement *versionNode = root->FirstChildElement("Version");
	if (versionNode)
	{
		const char *val = versionNode->GetText();
		if (val)
			_currentVersion = val;
	}

	XMLElement *paramNode = root->FirstChildElement("Param");
	if (paramNode)
	{
		const char *val = paramNode->GetText();
		if (val)
			_param = val;
	}
	
	XMLElement *infoURLNode = root->FirstChildElement("InfoUrl");
	if (!infoURLNode)
		throw exception("InfoUrl node is missed.");

	const char *iuVal = infoURLNode->GetText();
	if (!iuVal || !(*iuVal))
		throw exception("InfoUrl is missed.");
	
	_infoUrl = iuVal;

	XMLElement *classeNameNode = root->FirstChildElement("ClassName2Close");
	if (classeNameNode)
	{
		const char *val = classeNameNode->GetText();
		if (val)
			_className2Close = val;
	}

	XMLElement *progNameNode = root->FirstChildElement("MessageBoxTitle");
	if (progNameNode)
	{
		const char *valStr = progNameNode->GetText();
		if (valStr)
			_messageBoxTitle = valStr;

		valStr = progNameNode->Attribute("isModal");
		if (valStr)
		{
			if (stricmp(valStr, "yes") == 0)
				_isMessageBoxModal = true;
			else if (stricmp(valStr, "no") == 0)
				_isMessageBoxModal = false;
			else
				throw exception("isModal value is incorrect (only \"yes\" or \"no\" is allowed).");
		}

		int val = 0;
		if (progNameNode->QueryIntAttribute("extraCmd", &val) == XML_SUCCESS)
			_3rdButton_wm_cmd = val;

		if (progNameNode->QueryIntAttribute("ecWparam", &val) == XML_SUCCESS)
			_3rdButton_wParam = val;

		if (progNameNode->QueryIntAttribute("ecLparam", &val) == XML_SUCCESS)
			_3rdButton_lParam = val;

		const char *extraCmdLabel = progNameNode->Attribute("extraCmdButtonLabel");
		if (extraCmdLabel)
			_3rdButton_label = extraCmdLabel;
	}

	XMLElement *silentModeNode = root->FirstChildElement("SilentMode");
	if (silentModeNode)
	{
		const char *smnVal = silentModeNode->GetText();
		if (smnVal && *smnVal)
		{
			if (stricmp(smnVal, "yes") == 0)
				_isSilentMode = true;
			else if (stricmp(smnVal, "no") == 0)
				_isSilentMode = false;
			else
				throw exception("SilentMode value is incorrect (only \"yes\" or \"no\" is allowed).");
		}
	}

	
	//
	// Get optional parameters
	//
	XMLElement *userAgentNode = root->FirstChildElement("SoftwareName");
	if (userAgentNode)
	{
		const char *uaVal = userAgentNode->GetText();
		if (uaVal)
			_softwareName = uaVal;
	}
}

GupDownloadInfo::GupDownloadInfo(const char * xmlString) : _updateVersion(""), _updateLocation("")
{
	_xmlDoc.Parse(xmlString);

	XMLElement *root = _xmlDoc.FirstChildElement("GUP");
	if (!root)
		throw exception("It's not a valid GUP xml.");

	XMLElement *needUpdateNode = root->FirstChildElement("NeedToBeUpdated");
	if (!needUpdateNode)
		throw exception("NeedToBeUpdated node is missed.");

	const char *nunVal = needUpdateNode->GetText();
	if (!nunVal || !(*nunVal))
		throw exception("NeedToBeUpdated is missed.");
	
	if (stricmp(nunVal, "yes") == 0)
		_need2BeUpdated = true;
	else if (stricmp(nunVal, "no") == 0)
		_need2BeUpdated = false;
	else
		throw exception("NeedToBeUpdated value is incorrect (only \"yes\" or \"no\" is allowed).");

	if (_need2BeUpdated)
	{
		//
		// Get mandatory parameters
		//
		XMLElement *versionNode = root->FirstChildElement("Version");
		if (versionNode)
		{
			const char *val = versionNode->GetText();
			if (val)
				_updateVersion = val;
		}
		
		XMLElement *locationNode = root->FirstChildElement("Location");
		if (!locationNode)
			throw exception("Location node is missed.");

		const char *locVal = locationNode->GetText();
		if (!locVal || !(*locVal))
			throw exception("Location is missed.");
		
		_updateLocation = locVal;
	}
}

GupExtraOptions::GupExtraOptions(const char * xmlFileName) : _proxyServer(""), _port(-1)//, _hasProxySettings(false)
{
	_xmlDoc.LoadFile(xmlFileName);

	XMLElement *root = _xmlDoc.FirstChildElement("GUPOptions");
	if (!root)
		return;
		
	XMLElement *proxyNode = root->FirstChildElement("Proxy");
	if (proxyNode)
	{
		XMLElement *serverNode = proxyNode->FirstChildElement("server");
		if (serverNode)
		{
			const char *val = serverNode->GetText();
			if (val)
				_proxyServer = val;
		}

		XMLElement *portNode = proxyNode->FirstChildElement("port");
		if (portNode)
		{
			const char *val = portNode->GetText();
			if (val)
				_port = atoi(val);
		}
	}
}

void GupExtraOptions::writeProxyInfo(const char *fn, const char *proxySrv, long port)
{
	XMLDocument doc;
	XMLElement *root = doc.NewElement("GUPOptions");
	doc.InsertEndChild(root);
	XMLElement *proxy = doc.NewElement("Proxy");
	root->InsertEndChild(proxy);
	XMLElement *server = doc.NewElement("server");
	proxy->InsertEndChild(server);
	server->InsertEndChild(doc.NewText(proxySrv));
	XMLElement *portNode = doc.NewElement("port");
	proxy->InsertEndChild(portNode);
	char portStr[10];
	sprintf(portStr, "%d", port);
	portNode->InsertEndChild(doc.NewText(portStr));

	doc.SaveFile(fn);
}

std::string GupNativeLang::getMessageString(std::string msgID)
{
	if (!_nativeLangRoot)
		return "";

	XMLElement *popupMessagesNode = _nativeLangRoot->FirstChildElement("PopupMessages");
	if (!popupMessagesNode)
		return "";

	XMLElement *node = popupMessagesNode->FirstChildElement(msgID.c_str());
	if (!node)
		return "";

	const char *val = node->GetText();
	if (!val || !(*val))
		return "";
	
	return val;
}
