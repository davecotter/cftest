/*	kJams Project - main.js
	Description: Main screen functionality.
	(c) 2007 kJams and David M. Cotter	*/

var timer = null;
var lastPing = 0;

function m_populatePlaylists() {
	//Remove anything that was already there
	divObj = document.getElementById("playlists");
	
	for (var i = 0; i < divObj.childNodes.length; i++) {
		divObj.removeChild(divObj.childNodes[i]);
	}
	
	list = new KJDropList();
	list.create(divObj);
	playlistsList = list;
	
	getPlaylists(url_playlists, m_playlistsLoaded);
}

function m_playlistsLoaded(list) {
	listCtrl = document.getElementById("playlists").childNodes[0].parent;
	
	for (var i = 0; i < list.length; i++) {
		if ((list[i]['name'] == "Library") || (list[i]['name'] == "History"))
			dropTarget = false;
		else
			dropTarget = true;
		
		listCtrl.addItem("p" + list[i]['id'], list[i]['name'], dropTarget, null, false);
		
		//Library and History items are not rearrangeable
		if ((list[i]['name'] == "Library") || (list[i]['name'] == "History"))
			document.getElementById("p" + list[i]['id']).rearrange = false;
		else
			document.getElementById("p" + list[i]['id']).rearrange = true;
		
		//Keep track of ids for 'Tonight' and 'Favorites'
		if (list[i]['name'] == "Tonight") {
			pTonightID = list[i]['id'];
		}
		
		if (list[i]['name'] == "Favorites") {
			pFavoritesID = list[i]['id'];
		}
		
		//Store the playlist name in the DOM object
		document.getElementById("p" + list[i]['id']).name = list[i]['name'];
	}
	
	listCtrl.color();
	listCtrl.evtClick = m_populateSongs;
	listCtrl.evtDropInto = m_playlistDrop;
}

function m_populateSongs(playlist) {
	playlist = playlist.substring(1);
	
	//Hide
	if (songsList) {
		songsList.hide();
		
		if (songsList.playlist != "1") {
			songsList.destroy();
		}
	}
	
	//Hide "No Playlist" message, show waiting message
	document.getElementById("message").style.display = 'none';
	document.getElementById("msg_dosearch").style.display = 'none';
	document.getElementById("msg_error").style.display = 'none';
	document.getElementById("waiting").style.display = '';
	
	//If the list is already loaded, simply display it
	if ((playlist == "1") && (songsLists[playlist] != undefined)) {
		songsList = songsLists[playlist];
		songsList.show();
		document.getElementById("waiting").style.display = 'none';
	}
	
	//If it's the library, request that user do a search first
	else if (playlist == "1") {
		document.getElementById("waiting").style.display = 'none';
		document.getElementById("msg_dosearch").style.display = '';
	}
	
	else {
		//Playlist DOM object
		playlistObj = document.getElementById("p" + playlist);
		
		//Which columns are needed?
		var columns = Array("#", "Song Name", "Artist", "Album");
		
		if (playlistObj.name == "Tonight") {
			columns.push("Pitch");
			columns.push("Add to");
		}
		
		else if (playlistObj.name == "Favorites") {
			columns.push("Add to");
		}
		
		else {
			columns.push("Add to");
			columns.push("Add to");
		}
		
		songsList = new KJList();
		songsList.rearrange = playlistObj.rearrange;
		songsList.playlist = playlist;
		songsList.playlistName = playlistObj.name;
		songsList.create(document.getElementById("songs"));
		songsList.setColumns(columns, 0);
		songsList.evtDrop = m_songRearrange;
		songsLists[playlist] = songsList;
		getSongs(url_songs, m_songsLoaded, "playlist=" + playlist);
		
		if (false) {
			songsList.evtColClick = null;
		}
		else {
			songsList.evtColClick = m_sortSongs;
		}
	}
	
	m_sessionCheckin();
}

function m_songsLoaded(list) {
	//Hide waiting message
	document.getElementById("waiting").style.display = 'none';
	
	//Rearrangeable?
	if (songsList.rearrange)
		var rearrange = true;
	else
		var rearrange = false;
	
	for (var i = 0; i < list.length; i++) {
		//Add to favorites
		var favsButton = document.createElement("INPUT");
		favsButton.type = "button"
		favsButton.name = songsList.playlist + "-" + list[i]['itemId'];
		favsButton.songId = list[i]['id'];
		favsButton.value = "Favorites";
		favsButton.onmousedown = m_addToFavorites;
		
		//Add to Tonight
		var tonightButton = document.createElement("INPUT");
		tonightButton.type = "button";
		tonightButton.name = songsList.playlist + "-" + list[i]['itemId'];
		tonightButton.songId = list[i]['id']; 
		tonightButton.value = "Tonight";
		tonightButton.onmousedown = m_addToTonight;
	
		//Columns
		var columns = Array(i, list[i]['name'], list[i]['artist']);
		
		if (typeof(list[i]['album']) == "object") {
			var dropDown = document.createElement("SELECT");
			dropDown.name = list[i]['id'];
			
			for (var j = 0; j < list[i]['album']['items'].length; j++) {
				var optionObj = document.createElement("OPTION");
				optionObj.value = list[i]['album']['items'][j];
				optionObj.innerHTML = optionObj.value;
				
				if (list[i]['album']['default'] == j) {
					optionObj.selected = true;
				}
				
				dropDown.appendChild(optionObj);
			}
			
			columns.push(dropDown);
		}
		
		else {
			columns.push(list[i]['album']);
		}
		
		if (songsList.playlistName == "Tonight") {
			//Drop-down
			var options = {"+6" : 6, "+5" : 5, "+4" : 4, "+3" : 3, "+2" : 2, "+1" : 1, "0" : 0, "-1" : -1, "-2" : -2, "-3" : -3, "-4" : -4, "-5" : -5, "-6" : -6}
			var dropDown = document.createElement("SELECT");
			dropDown.name = list[i]['itemId'];
			dropDown.songId = list[i]['id'];
			dropDown.onchange = m_changePitch;
			for (option in options) {
				var optionObj = document.createElement("OPTION");
				optionObj.value = options[option];
				optionObj.innerHTML = option;
				
				if(list[i]['pitch'] == options[option]) {
					optionObj.selected = true;
				}
				
				dropDown.appendChild(optionObj);
			}
			
			columns.push(dropDown);
			columns.push(favsButton);
		}
		
		else if (songsList.playlistName == "Favorites") {
			columns.push(tonightButton);
		}
		
		else {
			columns.push(tonightButton);
			columns.push(favsButton);
		}
	
		index = songsList.addItem(songsList.playlist + "-" + list[i]['itemId'], columns, rearrange, null, false);
		
		document.getElementById(songsList.getByIndex(index)).songName	= list[i]['name'];
		document.getElementById(songsList.getByIndex(index)).songId		= list[i]['id'];
		document.getElementById(songsList.getByIndex(index)).piIx		= list[i]['itemId'];
	}
	
	songsList.color();
	
	if (songsList.playlistName != "Tonight" && songsList.playlistName != "History") {
		songsList.evtColClick = m_sortSongs;
	}
	else {
		songsList.evtColClick = null;
	}
}

function m_songRearrange(list, song, index, oldIndex) {
	sendData(url_rearrange, "playlist=" + list.playlist + "&index=" + index + "&oldIndex=" + oldIndex);
	m_sessionCheckin();
}

function m_playlistDrop(song, index, nodeId) {
	playlists	= document.getElementById("playlists").childNodes[0].parent;
	item		= songsList.getByID(song);
	
	if (songsLists[playlists.getByIndex(index).substring(1)]) { songsLists[playlists.getByIndex(index).substring(1)].destroy(); }
	songsLists[playlists.getByIndex(index).substring(1)] = null;
	sendData(url_drop, "playlist=" + playlists.getByIndex(index).substring(1) + "&song=" + songsList.getByID(song).songId);
	
	m_setStatus("Added \"" + item.songName + "\" to \"" + m_getPlaylistName(playlists.getByIndex(index)) + "\" ...");
	m_sessionCheckin();
}

function m_addToTonight(event) {
	playlists = document.getElementById("playlists").childNodes[0].parent;
	m_playlistDrop(event.target.name, playlists.getIndex("p" + pTonightID), event.target.name);
}

function m_addToFavorites(event) {
	playlists = document.getElementById("playlists").childNodes[0].parent
	m_playlistDrop(event.target.name, playlists.getIndex("p" + pFavoritesID), event.target.name);
}

function m_sortSongs(list, column) {
	list.removeAll();
	list.setPrimaryCol(column);
	document.getElementById("waiting").style.display = '';
	
	if (document.getElementById("searchfield").value) {
		search = "&search=" + document.getElementById("searchfield").value;
	}
	
	getSongs(url_sort, m_songsLoaded, "playlist=" + list.playlist + "&orderby=" + column + search);
	m_sessionCheckin();
}

function m_changePitch(event) {
	sendData(url_pitch, "song=" + event.target.songId + "&pitch=" + event.target.value);
	m_sessionCheckin();
}

function m_doSearch() {
	//Select "Library" in the side bar
	playlistsList.select(playlistsList.getByID(playlistsList.getByIndex(0)));
	
	//Remove old search list if it's there
	if (songsLists["1"]) {
		songsLists["1"].parentObj.removeChild(songsLists["1"].container);
		songsLists["1"].parentObj.removeChild(songsLists["1"].columnsObj);
	}
	
	//Hide currently loaded list
	if (songsList) {
		songsList.hide();
	}
	
	document.getElementById("msg_dosearch").style.display = 'none';
	document.getElementById("message").style.display = 'none';
	document.getElementById("msg_error").style.display = 'none';
	document.getElementById("waiting").style.display = '';

	var columns = Array("#", "Song Name", "Artist", "Album", "Add to", "Add to");
	
	songsList = new KJList();
	songsList.rearrange = false;
	songsList.playlist = "1";
	songsList.playlistName = "Library";
	songsList.create(document.getElementById("songs"));
	songsList.setColumns(columns, 1);
	songsLists["1"] = songsList;
	
	getSongs(url_search, m_songsLoaded, "search=" + document.getElementById("searchfield").value);
	m_sessionCheckin();
}

function searchKeyPressed(event) {
	if (event.keyCode == 13) {
		m_doSearch();
	}
}

function m_getSongName(id) {
	songObj = document.getElementById(id);
	return songObj.songName;
}

function m_getPlaylistName(id) {
	playlistObj = document.getElementById(id);
	return playlistObj.name;
}

function kj_data_error(id, desc) {
	songsLists[songsList.id] = null;
	songsList.hide();
	document.getElementById("waiting").style.display = 'none';
	
	document.getElementById("msg_error").innerHTML = "Error: " + desc
	document.getElementById("msg_error").style.display = '';
}

function m_setStatus(status, revert) {
	statusObj = document.getElementById("status");
	statusObj.innerHTML = status;
}

function m_keyPressed(event) {
	//This is not IE-compliant code.
	if (event.keyCode == 46) {
		if (!songsList.selection) {
			return;
		}
	
		playlistName	= songsList.playlistName;

		if (playlistName != "Library" && playlistName != "History") {
			selection	= songsList.selection;
			item		= songsList.getByID(selection);
			
			songsList.select(null);
			sendData(url_remove, "playlist=" + songsList.playlist + "&piIx=" + item.piIx);
			m_setStatus("Removed \"" + item.songName + "\" ...");
			songsList.removeItem(selection);
		}
	}
}

function m_pingSession() {
	var elapsedTime = new Date().getTime()/1000.0 - lastPing;
	
	if (elapsedTime >= 5) {
		sendData(url_ping, "");
		m_sessionCheckin();
		lastPing = new Date().getTime()/1000.0;
	}
}

function m_sessionCheckin() {
	/* A true or false value is set by the server before delivering this script 
		depending on preference. */
	if (!{auto_logout}) {
		return;
	}
	
	if (timer == null) {
		timer = setInterval("m_sessionLogout()", {timeout} * 1000);
	}
	else {
		clearInterval(timer);
		timer = null;
		return m_sessionCheckin();
	}
}

function m_sessionLogout() {
	window.location=url_login;
}
