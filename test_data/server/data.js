/*	kJams Project - data.js
	Description: Provides network functionality for the kJams server.
	(c) 2007 kJams and David M. Cotter	*/

//Send data; don't expect a response
function sendData(url, post) {
	loadData(url, function() { }, null, post);
}

//Load data from a script or XML file and callback to a function
function loadData(url, callback, additional, post) {
	if (post == undefined) {
		var post = "";
	}

	var data = new Spry.Data.XMLDataSet(url, "/plist/dict", {useCache : false, 'method' : 'POST', 'postData' : post, headers: { "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8" }});
	data.callback = callback;
	data.additional = additional;
	data.addObserver(dataLoaded);
	data.loadData();
}

function dataLoaded(notification, dataSet, data) {
	if (notification == "onPostLoad") {
		var result = dataSet.getDocument();
		dataSet.callback(result, dataSet.additional);
	}
}

function getSongs(url, callback, post) {
	loadData(url, songsLoaded, callback, post);
}

function songsLoaded(doc, callback) {
	var songs = [ ];
	var data = plist_parse(doc);
	
	//Error handling
	if (data['root']['Error']) {
		if (kj_data_error != undefined) {
			kj_data_error(data['root']['Error']['ID'], data['root']['Error']['Description']);
		}
		
		return;
	}
	
	var items = data['root']['Playlists'][0]['Playlist Items'];
	
	for (var i = 0; i < items.length; i++) {
		songs.push({'id' : items[i]['soID'], 'itemId' : items[i]['piIx'], 'name' : items[i]['name'], 'artist' : items[i]['arts'], 'album' : items[i]['albm'], 'pitch' : items[i]['PICH']});
	}
	
	callback(songs);
}

function getSingers(url, callback, post) {
	loadData(url, singersLoaded, callback, post);
}

function singersLoaded(doc, callback) {
	var singers = [ ];
	var data = plist_parse(doc);
	
	var items = data['root']['Playlists'][0]['Playlist Items'];
	
	for (var i = 0; i < items.length; i++) {
		singers.push({'id' : items[i]['siID'], 'name' : items[i]['SNGR']});
	}
	
	callback(singers);
}

function getPlaylists(url, callback, post) {
	loadData(url, playlistsLoaded, callback, post);
}

function playlistsLoaded(doc, callback) {
	var playlists = [ ];
	var data = plist_parse(doc);
	
	//Error handling
	if (data['root']['Error']) {
		if (kj_data_error != undefined) {
			kj_data_error(data['root']['Error']['ID'], data['root']['Error']['Description']);
		}
		
		return;
	}
	
	var items = data['root']['Playlists'][0]['Playlist Items'];
	
	for (var i = 0; i < items.length; i++) {
		playlists.push({'id' : items[i]['Playlist ID'], 'name' : items[i]['Name']});
	}
	
	callback(playlists);
}

//Turn a plist XML document interpreted by Spry into something easy to work with
function plist_parse(data) {
	var parsed = { };
	
	root = data.documentElement;
	
	for (var i = 0; i < root.childNodes.length; i++) {
		//Find root node
		if (root.childNodes[i].nodeName[0] != "#") {
			rootNode = root.childNodes[i];
			
			if (rootNode.nodeName == "dict") {
				parsed['root'] = plist_dict(rootNode);
			}
			
			else if (rootNode.nodeName == "array") {
				parsed['root'] = plist_array(rootNode);
			}
			
			else {
				parsed['root'] = rootNode.nodeValue;
			}
		}
	}
	
	return parsed;
}

function plist_dict(node) {
	var data = { };
	
	//Remove the annoying "#text" nodes.
	for (var i = 0; i < node.childNodes.length; i++) {
		if (node.childNodes[i].nodeName[0] == "#") {
			node.removeChild(node.childNodes[i]);
		}
	}

	for (var i = 0; i < node.childNodes.length; i++) {
		//Find the key
		if (node.childNodes[i].nodeName == "key") {
			var key = node.childNodes[i].firstChild.data;
			var value = node.childNodes[i].nextSibling;
			
			//Store the value
			if (value.nodeName == "dict") {
				data[key] = plist_dict(value);
			}
			
			else if (value.nodeName == "array") {
				data[key] = plist_array(value);
			}
			
			else {
//				alert(value.firstChild.data);
//				break;
				if (value.firstChild)
					data[key] = value.firstChild.data;
			}
		}
		
		//Skip the next node, since we've already delt with it
		i++;
	}

	return data;
}

function plist_array(node) {
	var data = [ ];
	
	//Remove the annoying "#text" nodes.
	for (var i = 0; i < node.childNodes.length; i++) {
		if (node.childNodes[i].nodeName[0] == "#") {
			node.removeChild(node.childNodes[i]);
		}
	}

	for (var i = 0; i < node.childNodes.length; i++) {
		var value = node.childNodes[i];
		
		//Store the value
		if (value.nodeName == "dict") {
			data.push(plist_dict(value));
		}
		
		else if (value.nodeName == "array") {
			data.push(plist_array(node));
		}
		
		else {
			data.push(value.firstChild.data);
		}
	}
	
	return data;
}
