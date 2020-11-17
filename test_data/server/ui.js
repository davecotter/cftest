/*	kJams Project - ui.js
	Description: Provides UI functionality for the kJams server.
	(c) 2007 kJams and David M. Cotter	*/
	
//Add indexOf() to the Array class, if not present (backwards compatibility)
if (!Array.prototype.indexOf)
{
  Array.prototype.indexOf = function(elt /*, from*/)
  {
    var len = this.length;

    var from = Number(arguments[1]) || 0;
    from = (from < 0)
         ? Math.ceil(from)
         : Math.floor(from);
    if (from < 0)
      from += len;

    for (; from < len; from++)
    {
      if (from in this &&
          this[from] === elt)
        return from;
    }
    return -1;
  };
}

//Keep a master array for all lists and their location and size info for event handling
var lists = new Array();

//Item mouse is currently positioned over
var active = null;

//Drop target
var target = null;

//Auto scroll item
var scrollItem = null;

//Mouse information
var mousePos = null;
var prevPos = null;
var buttonPressed = false;

//Logic information
var dragging = false;
var dragObject = null;

//Pass mouse events to custom methods
document.onmouseup = mouseUp;
document.onmousedown = mouseDown;
document.onmousemove = mouseMove;

function mouseUp(event) {
	buttonPressed = false;
	
	if (dragging) {
		//Stop scrolling
		if (dragObject.parent.scrolling)
			dragObject.parent.stopScrolling();
		
		//Drop
		if (target) {
			target.parent.dropItem(dragObject.parent.selection, target.parent.getIndex(target.parent.getByObject(target)));
			target.parent.unTarget();
		}
		
		//Release dragging
		dragging = false;
		dragObject.parent.container.removeChild(dragObject);
		dragObject = null;
	}
	
	if (event.target.isListItem == undefined) {
		return true;
	}
	
	else {
		return false;
	}
}

function mouseDown(event) {
	var active;
	buttonPressed = true;
	
	//Pass event onto item's parent object and begin dragging
	if (active = hitTest(getMousePos(event))) {
		active.parent.select(active);
		
		//Only drag if dragging is enabled
		if (!active.parent.enableDragging) { return false; }
		
		dragging = true;
		dragObject = active.cloneNode(true);
		dragObject.id = "dragObject";
		dragObject.onmouseover = function () { return true; }
		dragObject.parent = active.parent;
		dragObject.style.width = active.offsetWidth;
		dragObject.className = "KJListItem_drag";
		dragObject.style.display = "none";
 		dragObject.parent.container.appendChild(dragObject);
 		var activePos = getObjectPos(active);
		dragObject.mouseOffset = new Array(activePos[0] - active.parent.container.scrollLeft - mousePos[0], activePos[1] - active.parent.container.scrollTop - mousePos[1]);
		objectToMouse(dragObject, dragObject.mouseOffset);
	}
	
	if (event.target.isListItem == undefined) {
		return true;
	}
	
	else {
		return false;
	}
}

function mouseMove(event) {
	var active;
	var containerPos;
	mousePos = getMousePos(event);
	
	//Handle dragging
	if (dragging) {
		if (dragObject.style.display == "none") { dragObject.style.display = ""; }
		objectToMouse(dragObject, dragObject.mouseOffset);
		
		//Hit testing
		if (active = hitTest(mousePos)) {
			active.parent.makeTarget(active);
			
			containerPos = getObjectPos(active.parent.container);
			
			//Auto scrolling (down)
			if ((mousePos[1] + 40 >= containerPos[1] + active.parent.container.offsetHeight) && (prevPos) && (prevPos[1] < mousePos[1])) {
				if (active.parent.scrolling)
					active.parent.setScrollSpeed(40 - ((containerPos[1] + active.parent.container.offsetHeight) - mousePos[1]));
				else
					active.parent.startScrolling(40 - ((containerPos[1] + active.parent.container.offsetHeight) - mousePos[1]));
			}
			
			//Auto scrolling (up)
			else if ((mousePos[1] - 40 <= containerPos[1]) && (prevPos) && (prevPos[1] > mousePos[1])) {
				if (active.parent.scrolling)
					active.parent.setScrollSpeed(-40 - (containerPos[1] - mousePos[1]));
				else
					active.parent.startScrolling(-40 - (containerPos[1] - mousePos[1]));
			}
			
			//Stop scrolling
			else if (active.parent.scrolling)
				active.parent.stopScrolling();
		}
		
		else if (target) {
			target.parent.unTarget();
		}
	}
	
	prevPos = mousePos;
	
    return false;
}

function getMousePos(event) {
	if (event.pageX || event.pageY) {
		return new Array(event.pageX, event.pageY);
	}
	
	return new Array(	event.clientX + document.body.scrollLeft - document.body.clientLeft,
						event.clientY + document.body.scrollTop - document.body.clientTop	);
}

//Determine which object the mouse currently resides over
function hitTest(mousePos) {
	for(var i = 0; i < lists.length; i++) {
		var objectPos = getObjectPos(lists[i].container);
		
		//No clipping
		if (!lists[i].clip) {
			if ((objectPos[0] <= mousePos[0]) && (objectPos[0] + lists[i].container.offsetWidth >= mousePos[0]) && (objectPos[1] <= mousePos[1]) && (objectPos[1] + lists[i].container.offsetHeight >= mousePos[1])) {
				return lists[i].hitTest(new Array(mousePos[0] + lists[i].container.scrollLeft, mousePos[1] + lists[i].container.scrollTop));
			}
		}
		
		//Clipping
		else {
			if ((objectPos[0] + lists[i].clip['left'] <= mousePos[0]) && (objectPos[0] + lists[i].container.offsetWidth - lists[i].clip['right'] >= mousePos[0]) && (objectPos[1] + lists[i].clip['top'] <= mousePos[1]) && (objectPos[1] + lists[i].container.offsetHeight - lists[i].clip['bottom'] >= mousePos[1])) {
				return lists[i].hitTest(new Array(mousePos[0] + lists[i].container.scrollLeft, mousePos[1] + lists[i].container.scrollTop));
			}
		}
		
	}
	return false;
}

//Find an objects position
function getObjectPos(object) {
	var x = 0;
	var y = 0;
	var obj = object;
	
	while (obj.offsetParent) {
		x += obj.offsetLeft;
		y += obj.offsetTop;
		obj = obj.offsetParent;
	}
	
	x += obj.offsetLeft;
	y += obj.offsetTop;
	
	return new Array(x, y);
}

//KJList control
function KJList() { 
	this.visible = true;
	this.container = null;
	this.columnsObj = null;
}

//KJList class constructor; if provided with an object, the list will be contained within; otherwise, it'll just be on the page.
KJList.prototype.create = function(object) {
	if(!object) {
		object = document.body;
	}
	
	//Parent Object
	this.parentObj = object;
	
	//Items list
	this.items = new Array();
	
	//Item selected
	this.selection = null;
	
	//Item mouse is currently positioned over
	this.active = null;
	
	//Items are dragable
	this.enableDragging = true;
	
	//Container
	this.container = document.createElement('DIV');
	this.container.className = "KJList";
	this.container.parent = this;
	object.appendChild(this.container);
	
	//Visible?
	if (!this.visible)
		this.container.style.display = 'none';
	
	//Columns
	this.columns = null;
	this.colWidths = Array();
	this.primaryCol = null;
	this.columnsObj = null;
	this.sortBy = null;
	
	//Add self to the array
	lists.push(this);
	
	//Self-assign an id based on list position
	this.id = String(lists.length - 1);
	
	//Hit-test clipping
	this.clip = null;
	
	//Event handlers
	this.evtClick = null;
	this.evtDrop = null;
	this.evtDropInto = null;
	this.evtColClick = null;
};

KJList.prototype.destroy = function() {
	//Remove all DOM items
	this.removeAll()
	
	//Remove columns
	if (this.columnsObj.parentNode != null) {
		this.columnsObj.parentNode.removeChild(this.columnsObj);
	}
	
	//Remove self
	if (this.container.parentNode != null) {
		this.container.parentNode.removeChild(this.container);
	}
}	

KJList.prototype.setClipping = function(boundries) {
	if (boundries['left'] == undefined)
		boundries['left'] = 0;
		
	if (boundries['right'] == undefined)
		boundries['right'] = 0;
		
	if (boundries['top'] == undefined)
		boundries['top'] = 0;
		
	if (boundries['bottom'] == undefined)
		boundries['bottom'] = 0;
		
	this.clip = boundries;
}

//Set alternating background colors
KJList.prototype.color = function() {
	var alt = false;
	for (var i = 0; i < this.container.childNodes.length; i++) {
		if (this.container.childNodes[i].className != "KJListItem_selected") {
			if (alt) {
				this.container.childNodes[i].className = "KJListItem_alt";
				alt = false;
			}
			else {
				this.container.childNodes[i].className = "KJListItem";
				alt = true;
			}
		}
		else {
			if (alt)
				alt = false;
			else
				alt = true;
		}
	}
};

KJList.prototype.setColumns = function(columns, primary) {
	//Primary column required
	if (primary == null) {
		return -1;
	}
	
	//If we're resetting the columns, remove the old ones first
	if (this.columnsObj) {
		this.parentObj.removeChild(columnsObj);
		this.colWidths = Array();
	}
		
	//Array
	this.columns = columns;
		
	//Table
	this.columnsObj = document.createElement('TABLE');
	this.columnsObj.className = "KJListColumn";
	
	//Visible?
	if (!this.visible)
		this.columnsObj.style.display = 'none';
	
	//Row
	var row = document.createElement('TR');
	this.columnsObj.appendChild(row);
	
	var columnItem;
	
	for (var i = 0; i < columns.length; i++) {
		columnItem = document.createElement('TD');
		columnItem.id = this.id + "-c" + String(i);
		columnItem.innerHTML = columns[i];
		columnItem.parent = this;
		columnItem.onclick = this.colClicked;
		row.appendChild(columnItem);
	}
	
	this.parentObj.insertBefore(this.columnsObj, this.container);
	this.columnsObj.style.width = this.container.offsetWidth;
	
	//Set initial column widths
	for (var i = 0; i < row.childNodes.length; i++) {
		this.colWidths.push(row.childNodes[i].offsetWidth);
	}
	
	//Set primary column
	this.setPrimaryCol(primary);
}

KJList.prototype.getColumnWidth = function(colID) {
	return this.colWidths[colID];
}

//Add an item to the list at the bottom (or given index); returns the item's index.
KJList.prototype.addItem = function(id, label, target, index, recolor) {
	if (!index) {
		index = this.items.length;
	}
	
	if (target == null) {
		target = true;
	}
	
	//Default column
	if (!this.columns) {
		this.columns = Array(1);
		this.primaryCol = 0;
		this.colWidths = Array("100%");
	}
	
	if (typeof(label) != "object") {
		labelArray = Array();
		labelArray[this.primaryCol] = label;
		label = labelArray;
	}
	
	if (recolor == null) {
		recolor = true;
	}
	
	//Typecast the id as a string
	id = String(id);
	
	//Add the item to the list
	this.items.splice(index, 0, id);
	
	//Div
	item = document.createElement('DIV');
	
	//Columns
	var colTable = document.createElement('TABLE');
	var colRow = document.createElement('TR');
	var colItem = null;
	
	for (var i = 0; i < this.columns.length; i++) {
		if (label[i] == undefined)
			label[i] = "";
			
		colItem = document.createElement('TD');
		colItem.isListItem = true;

		if (typeof(label[i]) == "object")
			colItem.appendChild(label[i]);
		else
			colItem.innerHTML = label[i];

		colItem.style.width = this.colWidths[i];
		colRow.appendChild(colItem);
	}	
	
	colTable.appendChild(colRow);
	item.appendChild(colTable);

	item.id = id;
	item.className = "KJListItem";
	item.target = target;
	item.parent = this;
	
	if (index == this.items.length - 1) {
		this.container.appendChild(item);
	}
	
	else {
		indexObject = document.getElementById(this.items[index]);
		this.container.insertBefore(item, indexObject);
	}
	
	if (recolor)
		this.color();
	
	return index;
};

//Remove item by id
KJList.prototype.removeItem = function(id) {
	index = this.getIndex(id);
	this.removeAtIndex(index);
};

//Remove item at index
KJList.prototype.removeAtIndex = function(index) {
	item = this.getByID(this.items[index]);
	this.container.removeChild(item);
	this.items.splice(index, 1);
	
	//Recolor the list
	this.color();
};

KJList.prototype.removeAll = function() {
	listLength = this.items.length;
	
	for (var i = 0; i < listLength; i++) {
		this.removeAtIndex(0);
	}
};

//Move an item to a given position
KJList.prototype.moveItem = function(item, index) {
	var itemIndex = this.getIndex(item);
	
	//Move graphically
	indexObject = this.getByID(this.items[index]);
	this.container.insertBefore(this.getByID(item), indexObject);
	
	//Move logically
	this.items.splice(itemIndex, 1);
	if (itemIndex < index)
		this.items.splice(index - 1, 0, item);
	else
		this.items.splice(index, 0, item);
	
	//Recolor the list
	this.color()
};

//Given an item ID, grab the DOM object
KJList.prototype.getByID = function(id) {
	return document.getElementById(id);
};

KJList.prototype.getByIndex = function(index) {
	return this.items[index];
};

//Given a DOM object, grab the item id
KJList.prototype.getByObject = function(object) {
	return object.id;
};

//Given an item ID, return the index in the items array
KJList.prototype.getIndex = function(id) {
	return this.items.indexOf(id);
};


KJList.prototype.show = function() {
	if (!this.visible) {
		if (this.container)
			this.container.style.display = "";
		if (this.columnsObj)
			this.columnsObj.style.display = "";
		this.visible = true;
	}
}

KJList.prototype.hide = function() {
	if (this.visible) {
		if (this.container)
			this.container.style.display = "none";
		if (this.columnsObj)
			this.columnsObj.style.display = "none";
		this.visible = false;
	}
}

// --- //

KJList.prototype.select = function(item) {
	//First deselct other items
	if (this.selection) {
		this.getByID(this.selection).className = "KJListItem";
	}
	
	if (item != null) {
		this.selection = this.getByObject(item);
		item.className = "KJListItem_selected";
	}
	
	else {
		this.selection = null;
	}
	
	//Recolor the list
	this.color();
	
	if (this.evtClick)
		this.evtClick(this.selection);
};

KJList.prototype.setPrimaryCol = function(index) {
	columnObj = document.getElementById(this.id + "-c" + index);
	
	if (this.primaryCol != null) {
		primaryObj = document.getElementById(this.id + "-c" + this.primaryCol);
		primaryObj.className = "";
	}
	
	columnObj.className = "primary";
	
	this.primaryCol = index;
}

KJList.prototype.makeTarget = function(item) {
	if (item == target) { return; }
	if (target) { this.unTarget(); }
	if (!item.target) { return; }
	
	if (this.getByObject(item) != this.selection) {
		target = item;
		if (item.className == "KJListItem")
			item.className = "KJListItem_target";
		else if (item.className == "KJListItem_alt")
			item.className = "KJListItem_alt_target";
	}
};

KJList.prototype.unTarget = function() {
	if (this.getByObject(target) == this.selection) {
		target.className = "KJListItem_selected";
	}
	
	else {
		if (target.className == "KJListItem_target") {
			target.className = "KJListItem"; }
		else if (target.className == "KJListItem_alt_target")
			target.className = "KJListItem_alt";
			
		target = null;
	}
};

KJList.prototype.dropItem = function(item, index) {
	oldIndex = this.getIndex(item);
	this.moveItem(item, index);
	
	if (this.evtDrop != null) {
		this.evtDrop(this, item, index, oldIndex);
	}
};

KJList.prototype.colClicked = function(evt) {
	if (evt.target.parent.evtColClick != null) {
		evt.target.parent.evtColClick(evt.target.parent, evt.target.id.substring(evt.target.id.indexOf("-") + 2));
	}
}

KJList.prototype.hitTest = function(mousePos) {
	var object = null;
	
	if (this.items.length > 1) {
		var searchPoint = Math.round(this.items.length / 2);
	}
	else {
		var searchPoint = 0;
	}
	
	var prevPoint = searchPoint;
	var highPoint = this.items.length - 1;
	var lowPoint = 0;
	var objectPos = null;
	
	while (1) {
		object = this.getByID(this.items[searchPoint]);
		objectPos = getObjectPos(object);
		
		if (mousePos[1] < objectPos[1]) {
			if ((prevPoint == searchPoint - 1) || (prevPoint == 0))
				break;
				
			prevPoint = searchPoint;
			searchPoint = searchPoint - Math.round((searchPoint - lowPoint)/2);
			highPoint = prevPoint;
			continue;
		}
		
		else if (mousePos[1] > objectPos[1] + object.offsetHeight) {
			if ((prevPoint == searchPoint + 1) || (prevPoint == this.items.length - 1))
				break;
				
			prevPoint = searchPoint;
			searchPoint = searchPoint + Math.round((highPoint - searchPoint)/2);
			lowPoint = prevPoint;
			continue;
		}
		
		else if (mousePos[0] > objectPos[0] && mousePos[0] < objectPos[0] + object.offsetWidth) {
			return object;
		}
		
		else {
			return false;
		}
	}
	
	return false;
};

KJList.prototype.startScrolling = function(speed) {
//	document.getElementById("title").innerHTML = "Scrolling."
	this.scrollSpeed = speed;
	this.scrolling = true;
	scrollItem = this;
	scroll();
};

KJList.prototype.stopScrolling = function() {
//	document.getElementById("title").innerHTML = "Not scrolling.";
	this.scrolling = false;
};

KJList.prototype.setScrollSpeed = function(speed) {
	this.scrollSpeed = speed;
};

//Non-dragable list with drop targets; i.e., the playlists
function KJDropList() { }

//Inherit from KJList
KJDropList.prototype = new KJList();

//Constructor
KJDropList.prototype.create = function(object) {
	this.create = KJList.prototype.create;
	this.create(object);
	
	//Disable dragging
	this.enableDragging = false;
}

KJDropList.prototype.dropItem = function(item, index) {
	if (this.evtDropInto != null) {
		this.evtDropInto(item, index);
	}
}

function scroll() {
	if (scrollItem.scrolling) {
		scrollItem.container.scrollTop += scrollItem.scrollSpeed;
		setTimeout("scroll()", 100);
	}
}

//Move specified object to current mouse position
function objectToMouse(object, offset) {
	object.style.left = mousePos[0] + offset[0];
	object.style.top = mousePos[1] + offset[1];
}
