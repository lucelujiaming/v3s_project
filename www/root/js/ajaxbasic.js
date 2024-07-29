/*
 * Basic AJAX and XML parsing
 */

function getDataFromTag(xmlDoc, tag)
{
	try 
	{
		if (xmlDoc.getElementsByTagName(tag)[0] )
		{
			return xmlDoc.getElementsByTagName(tag)[0].firstChild.nodeValue;
		}
		return "";
	}
	catch (err)
	{
		return "";
	}
}

function getDataFromTagWithDefault(xmlDoc, tag)
{
	try 
	{
		if (xmlDoc.getElementsByTagName(tag)[0] )
		{
			return xmlDoc.getElementsByTagName(tag)[0].firstChild.nodeValue;
		}
		return "";
	}
	catch (err)
	{
		return "-";
	}
}

function hasError(xmlDoc)
{
	if(xmlDoc)
	{
		var errors = xmlDoc.getElementsByTagName("Error");		
		if (errors.length)
		{
			return 1;
		}	
		return 0;
	}
	return 0;
}

function showError(xmlDoc)
{
	var errors = xmlDoc.getElementsByTagName("Error");
	for (var i=0; i<errors.length; i++)
	{
		alert("Error: " + errors.item(i).firstChild.nodeValue);
	}
}

function XHRGet(handle, url, async, unique) 
{
	var request = null;
	var currentDate = new Date();
	var currentTime = currentDate.getTime();
	var urlUnique = url + "&T=" + currentTime;
	if (window.ActiveXObject) 
	{
		request = new ActiveXObject("Microsoft.XMLHTTP");
	} 
	else if (window.XMLHttpRequest) 
	{
		request = new XMLHttpRequest();
	}

	if (request == null)
	{
		alert("Your browser does not support XMLHttp");
		return -1;
	}
	
	if (handle != null)
	{
		request.onreadystatechange = function () { XHRGetHandle(request, handle); };
	}
	
	if (unique == undefined || unique == true)
	{
		request.open("GET", urlUnique, async);
	}
	else
	{
		request.open("GET", url, async);
	}
	request.send(null);
	return 0;
}

function XHRGetHandle(request, handle)
{
	if(request.readyState == 4 && request.status == 200) 
	{
		handle(request.responseXML);
	}
}

function XHRPost(handle, url, param, async) 
{
	var request = null;
	var currentDate = new Date();
	var currentTime = currentDate.getTime();
	var param = param + "&T=" + currentTime;
	if (window.ActiveXObject) 
	{
		request = new ActiveXObject("Microsoft.XMLHTTP");
	} 
	else if (window.XMLHttpRequest) 
	{
		request = new XMLHttpRequest();
	}

	if (request == null)
	{
		alert("Your browser does not support XMLHttp");
		return -1;
	}
	
	request.open("POST", url, async);
	request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	if (handle != null)
	{
		request.onreadystatechange = function () { XHRPostHandle(request, handle); };
	}
	request.send(param);
	return 0;
}

function XHRPostHandle(request, handle)
{
	if(request.readyState == 4 && request.status == 200) 
	{
		handle(request.responseXML);
	}
}

function serializeFormElement(form)
{
	var str = "";
	var elem = document.getElementById(form).elements;
	for(var i = 0; i < elem.length; i++)
	{
		if (elem[i].id && elem[i].value)
		{
			if (i!=0)
			{
				str+="&";
			}
			str +=elem[i].id + "=" + elem[i].value;
		}
	} 
	return str;
}
