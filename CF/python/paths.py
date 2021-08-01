import os, stat, shutil, sys, xplat, subprocess

#####################################################
def	file_delete(fsObj):
	if os.path.exists(fsObj):
		if os.path.isdir(fsObj):
			shutil.rmtree(fsObj, True)
		else:
			os.remove(fsObj)

#####################################################
def reveal(path):
	cmd = []

	if xplat.is_mac():
		cmd += ["open", "-R"]
	else:
		cmd += ["explorer", "/select,"]
		
	cmd += [path]
	subprocess.Popen(cmd)

#####################################################
def debug_print(str):
	# print "paths: " + str
	pass

def debug_print1(str):
	# debug_print(str)
	pass

def	get_script_path(fileStr):
	scriptPath = fileStr
	if scriptPath.find(os.sep) == -1:
		scriptPath = os.getcwd() + os.sep + scriptPath
	return scriptPath

def	get_script_folder(cur_script_path):
	debug_print1("cur_script_path: " + cur_script_path)
	
	scriptFile = os.path.abspath(cur_script_path)
	debug_print1("abs scriptFile : " + scriptFile)
	
	pathStr = get_script_path(scriptFile)
	debug_print1("pathStr        : " + pathStr)
	
	pathStr = os.path.dirname(pathStr)
	debug_print1("pathStr dir    : " + pathStr)
	
	absPath = os.path.abspath(pathStr) + os.path.sep
	debug_print1("absPath        : " + absPath)

	return absPath

def must_exist(filePath):
	if not os.path.exists(filePath):
		filePath = os.path.abspath(filePath)
		raise OSError("file not found: " + filePath)

#####################################################
# see qt/kjams/kjams.pri, DIR_BOOST
# should match!
def get_boost_vers():
	# only used for Qt builds
	# since the xcode proj has it specified in search paths in the GUI(!)
	'''
		note the xcode project copies "atomic" into the bungle even though it's
		64bit (not fat 32/64).  it's possible we do not need it? but i haven't tried
		removing it. we don't link with it so probably(?) dont need it?
	'''
	return '1_69_0'

# see qt/kjams/kjams.pri, DIR_PADDLE
# should match!
def get_paddle_framework_vers():
	# mac only
	return '4.0.15'

def get_vs_year():
	return '2019'

def get_vcRedist():
	vcRedistDir	= '14.26.28720/'
	return vcRedistDir

def get_win_sdk_vers():
	sdkVers		= '10.0.17763.0'
	return sdkVers

def get_program_files_dir():
	return 'C:/Program Files (x86)/'

def get_win10_kit_path():
	return get_program_files_dir() + 'Windows Kits/10/'

def get_win_sdk_path():
	path = get_win10_kit_path()
	path += 'bin' + os.path.sep
	path += get_win_sdk_vers()
	path += os.path.sep
	return path

###################################################
def get_qt_vers():
	qt_IDE_folder = get_qt_IDE_folder() + 'Qt/'
	dirList = os.listdir(qt_IDE_folder)
	#print('dir list:', dirList)
	dirList = [curName for curName in dirList if unicode(curName[0], "utf-8").isnumeric()]
	dirList.sort()
	#print('numeric list:', dirList)
	vers = dirList[-1]

	# temp: i installed a later SDK but haven't switched to it yet
	# vers = '5.14.2'

	return vers

def get_qt_IDE_folder(sdkVersFolderB = False):
	# return the folder that "Qt Creator" IDE folder is contained in
	# if True, then go into the SDK version folder
	
	qt_IDE_folder = ''
	
	if not xplat.is_mac():
		qt_IDE_folder	+= 'C:'
		
	qt_IDE_folder	+= '/Users/davec/developer/'

	if sdkVersFolderB:
		qt_IDE_folder += 'Qt/' + get_qt_vers() + os.path.sep
	
	return qt_IDE_folder

def get_IDE_vers():
	vers = get_qt_vers()
	vers = vers.replace('.', '_')
	vers = '-Desktop_Qt_' + vers + '_'
	return vers;

def is_debug(str):
	return str.lower() == 'debug'

def is_release(str):
	return str.lower() == 'release'

def convert_debug_case(str, upperB = True):
	is_debugB = is_debug(str)
	
	if is_debugB:
	
		if upperB:
			str = 'Debug'
		else:
			str = 'debug'
	else:
		is_releaseB = is_release(str)

		if is_releaseB:
	
			if upperB:
				str = 'Release'
			else:
				str = 'release'
		else:
			raise OSError("build type neither debug nor release")

	return str

def	get_win_folder(_file):
	pathStr = os.path.abspath(get_script_folder(_file) + '../proj/win/')
#	print "win folder: " + pathStr
	return pathStr

def	get_mac_folder(_file):
	pathStr = os.path.abspath(get_script_folder(_file) + '../../')
#	print "mac folder: " + pathStr
	return pathStr

def copy_file(src_file, dst_object):
	write_perm = stat.S_IWRITE | stat.S_IREAD | stat.S_IROTH
	base_name = os.path.basename(src_file)
#	print 'copying file: <' + base_name + '>'
#	print 'copy   it to: <' + dst_object + '>'	
	shutil.copy(src_file, dst_object)
	
	if os.path.isfile(dst_object):
#		print 'file --> ' + dst_object

		is_exeB = False
		if os.access(src_file, os.X_OK):
			is_exeB = True
			write_perm |= stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
		
		perms = os.stat(dst_object)
		os.chmod(dst_object, perms.st_mode | write_perm)

	else:
		new_dst_obj = dst_object + os.sep + base_name
		new_dst_obj.replace('/', os.sep)
#		print 'fldr --> ' + new_dst_obj
		os.chmod(new_dst_obj, write_perm)

def copy_folder(src_dir, dst_dir):
#	print 'copying folder: <' + src_dir + '>'
#	print 'copying  it to: <' + dst_dir + '>'
	
	if not os.path.exists(dst_dir):
		#print 'creating dest dir: ' + dst_dir
		os.makedirs(dst_dir)
	
#	print 'Default encoding: ' + sys.getdefaultencoding()
#	print 'FileSys encoding: ' + sys.getfilesystemencoding()
	
#	u_src_dir = unicode(src_dir)
	fileList = os.listdir(src_dir)
#	print 'found fileList:'
#	print fileList

	for fileName in fileList:
		#if not xplat.is_mac():
		#	fileName = unicode(fileName.replace(u'\xa7', u'\x15'))
		
		#filenName = unicode(fileName, 'latin1')
		#print fileName
		
		srcPath = src_dir + '/' + fileName
		new_dst_obj = dst_dir + '/' + fileName
		
	#	print 'copying to: ' + new_dst_obj
		
		if os.path.isdir(srcPath):
#			print '<' + fileName + '> is a folder'
			copy_folder(srcPath, new_dst_obj)
		else:
#			print '<' + fileName + '> is a file'
			copy_file(srcPath, new_dst_obj)

# src_dir has a trailing /, dst_dir does NOT
def merge_langs(stringFileName, src_dir, dst_dir):
#	print 'copying folder: <' + src_dir + '>'
#	print 'copying  it to: <' + dst_dir + '>'
	
	must_exist(dst_dir)
	
#	print 'Default encoding: ' + sys.getdefaultencoding()
#	print 'FileSys encoding: ' + sys.getfilesystemencoding()
	
#	u_src_dir = unicode(src_dir)
	fileList = os.listdir(src_dir)
#	print 'found fileList:'
#	print fileList

	for fileName in fileList:		
		srcPath = src_dir + fileName
		
		if os.path.isdir(srcPath):
			stringFileNAmeStr = '/' + stringFileName + '.strings'
			srcPath += stringFileNAmeStr
			new_dst_obj = dst_dir + '/' + fileName + stringFileNAmeStr
			
		#	print 'copying to: ' + new_dst_obj
			
			if os.path.isdir(srcPath):
	#			print '<' + fileName + '> is a folder'
				raise OSError("should be no folders in a lang folder")
			else:
	#			print '<' + fileName + '> is a file'
				copy_file(srcPath, new_dst_obj)

if __name__ == "__main__":
	print get_qt_vers()
