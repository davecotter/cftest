import sys, shutil, os, stat, glob, platform
import paths, xplat, set_write_perms

def debug_print(str):
	#print "post_build_cf: " + str
	pass

# this is the DEST exe / app
def get_exe_path(appPath):
	appPath = appPath[:-1]

	debug_print('appPath: ' + appPath)

	exeName = os.path.basename(appPath)
	exeName = exeName[:-4]
	
	debug_print('exeName: ' + exeName)

	if xplat.is_mac():
		exeDir = 'MacOS/'
	else:
		exeDir = 'Windows/'

	debug_print('exeDir: ' + exeDir)
	exePath = get_bundle_folder(appPath, exeDir)
	
	exePath += exeName;
	
	if not xplat.is_mac():
		exePath += '.exe'
	
	debug_print('exePath: ' + exePath)
	return exePath

def get_plat_depth_str(bitDepth):
	str = ''
	
	if xplat.is_mac():
		str += 'mac'
	else:
		str += 'win'

	str += '_' + bitDepth
	return str

def get_src_exe(buildType, bitDepth, projName, extension = ''):
	buildType	= buildType.lower()
	buildName	= projName
	is_kjamsB	= "kJams" in buildName
	is_cfB		= "CFTest" in buildName

	'''
	post_build_cf: delete_src_exe: CFTest, 64, Debug
	post_build_cf: params: 
		 buildType: debug
		 bitDepth: 64
		 projName: CFTest
		 buildName: CFTest

	post_build_cf: relative src_exe: win-CFTest/CFTest.exe
	post_build_cf: Z:\CFTest\project/qt/win-CFTest/CFTest.exe
	'''

	if False:
		debug_print("params: \n" + \
			"\t buildType: "	+ buildType		+ "\n"	\
			"\t bitDepth: "		+ bitDepth		+ "\n"	\
			"\t projName: "		+ projName		+ "\n"	\
			"\t buildName: "	+ buildName		+ "\n"	\
			)

	src_exe = ""

	if is_kjamsB:
		src_exe += "kJams/"

	if is_cfB:
		src_exe += "CFTest/"

	src_exe += get_plat_depth_str(bitDepth)
	src_exe += '-' + buildName + '/' + projName + extension

	debug_print("relative src_exe: " + src_exe)
	return src_exe

#####################################################################################
# buildType in this context is "debug | release"
def create_bundle(qtB, buildType, bitDepth, src_exePath, appPath, ssl_edition='openssl'):
	buildType = buildType.lower()
	
	if appPath.startswith("~"):
		 os.path.expanduser(appPath)

	appPath = os.path.abspath(appPath) + '/'
	debug_print('appPath: ' + appPath)

	debug_print('ssl_edition: ' + ssl_edition)

	# delete previous app
	if os.path.exists(appPath):
		debug_print('deleting previous')
		shutil.rmtree(appPath, True)
	else:
		debug_print('previous does not exist')

	print 'Building bundle...'
	print '	copying CFLite dlls'

	cfDir	= paths.get_script_folder(__file__)
	cfDir	= os.path.abspath(cfDir + '../opencflite-476.17.2') + '/'
	
	if qtB:
		if 0:
			# when i use 57, the app crashes while trying to load DLLs
			# but only in the 64bit version
			# maybe we're missing a runtime lib?
			icuVers = '57'
		else:
			icuVers = '40'
	else:
		icuVers = '40'
	
	debug_print('getting windows bundle dir')
	dest_execDir		= get_bundle_folder(appPath, 'Windows/')
	debug_print('got windows bundle dir')
	dest_charsetsDir	= dest_execDir	+ 'CoreFoundation.resources/CharacterSets'
	icuDir				= cfDir			+ 'icu'
	sslDepth			= xplat.bid_depth_to_arch(bitDepth)

	if icuVers != '40':
		icuDir += icuVers

	icuDir	+= '/bin'

	if bitDepth == '64':
		icuDir += bitDepth
	
	icuDir += '/'
	
	debug_print("icuDir: " + icuDir)
	
	dllName		= 'CFLite'
	dllPath		= cfDir
	dllExt		= '.dll'
	
	if paths.is_debug(buildType):
		dllName += ' ' + paths.convert_debug_case(buildType)

	debug_print('buildType: ' + buildType)
	debug_print('dllName: "' + dllName + dllExt + '"')

	if qtB:
		dllPath	+= 'Qt/CFLite/' + get_src_exe(buildType, bitDepth, dllName, dllExt)
	else:
		dllPath	+= 'dist/bin/' + dllName + dllExt

	debug_print("dllPath: " + dllPath)

	if not os.path.exists(dest_charsetsDir):
		os.makedirs(dest_charsetsDir)

	shutil.copy(dllPath, dest_execDir)

	# core foundation
	print '	copying CFLite ICU files'
	shutil.copy(icuDir + 'icudt' + icuVers + '.dll', dest_execDir)
	shutil.copy(icuDir + 'icuin' + icuVers + '.dll', dest_execDir)
	shutil.copy(icuDir + 'icuuc' + icuVers + '.dll', dest_execDir)

	print '	copying CFLite charsets'
	shutil.copy(cfDir + "CFCharacterSetBitmaps.bitmap", dest_charsetsDir)
	shutil.copy(cfDir + "CFUniCharPropertyDatabase.data", dest_charsetsDir)
	shutil.copy(cfDir + "CFUnicodeData-B.mapping", dest_charsetsDir)
	shutil.copy(cfDir + "CFUnicodeData-L.mapping", dest_charsetsDir)

	# SSL
	print '	copying ' + ssl_edition + ' dlls'
	openSSLDir	= os.path.abspath(cfDir + '../' + ssl_edition + '/' + sslDepth) + '/'
	debug_print("openSSLDir: " + openSSLDir)
	
	if (ssl_edition == 'openssl'):
		ssl_dll_list = ['libeay32MD', 'ssleay32MD']
	else:
		ssl_dll_list = ['libcrypto-41', 'libssl-43', 'libtls-15']
	
	for lib in ssl_dll_list:
		shutil.copy(openSSLDir + lib + ".dll", dest_execDir)

	# exe
	print '	copying executable...'
	if src_exePath != "":
		debug_print("src_exePath: " + src_exePath)
		shutil.copy(src_exePath, dest_execDir)

def get_app_path(buildFolderParent, buildType, bitDepth, projName):
	is_kjamsB		= "kJams" in projName
	dest_appPath	= buildFolderParent + 'build/'
	is_qtB			= os.path.basename(os.path.normpath(buildFolderParent)) == 'qt'

	# how did this ever work for kJams Qt ??
	if is_qtB:
		dest_appPath += get_plat_depth_str(bitDepth) + '/'

	dest_appPath	+= projName + '.app/'
	debug_print("dest_appPath: " + dest_appPath)
	
	return dest_appPath
	
# projName is "edition" (kJams Lite, Pro, 2)
# buildType is 'debug / release'
def create_bundle_qt(buildFolderParent, buildType, bitDepth, projName, ssl='openssl'):
	dest_appPath	= get_app_path(buildFolderParent, buildType, bitDepth, projName)

	src_exe			= buildFolderParent + get_src_exe(buildType, bitDepth, projName, '.exe')
	debug_print("src_exe: " + src_exe)

	# create bundle, copy CF stuff
	create_bundle(True, buildType, bitDepth, src_exe, dest_appPath, ssl)

def get_edition(projName):
	edition = ""
	
	if 'Lite' in projName:
		edition = 'Lite'
	elif 'Pro' in projName:
		edition = 'Pro'
	elif '2' in projName:
		edition = '2'
	else:
		raise OSError("invalid kJams edition: " + projName)

	return edition

def	rename_hungarian(resPath):
	newName = resPath + 'hu_US.lproj'

	if os.path.exists(newName):
		shutil.rmtree(newName, True)

	os.rename(resPath + 'Hungarian.lproj', newName);

def get_bundle_folder(appPath, bundleDir):
	debug_print('appPath: ' + appPath);
	debug_print('bundleDir: ' + bundleDir);

	if (appPath[-1] != '/'):
		appPath += '/'
	
	if bundleDir:
		if (bundleDir[-1] != '/'):
			bundleDir += '/'
		
	return appPath + 'Contents/' + bundleDir

def copy_to_bundle(buildFolderParent, buildType, bitDepth, projName, destBundleDir, srcFiles):
	copyStr = '\tContents/'
	
	if destBundleDir != '':
		copyStr += destBundleDir
	
	copyStr += '...'
	print copyStr
	
	dest_appPath = get_app_path(buildFolderParent, buildType, bitDepth, projName)

	debug_print('destBundleDir: ' + destBundleDir)	
	destFolder	= get_bundle_folder(dest_appPath, destBundleDir)

	if not os.path.exists(destFolder):
		os.makedirs(destFolder)

	set_write_perms.shutil_chmod_folder(destFolder)

	for srcFile in srcFiles:
		fileName = os.path.basename(os.path.normpath(srcFile))
		print "\t\t" + fileName
				
		if os.path.isdir(srcFile):
			debug_print("srcFolder:  " + srcFile)
			debug_print("destFolder: " + destFolder)
			
			extensionStr = os.path.splitext(fileName)[1]

			if extensionStr == '.framework' or extensionStr == '.app':
				destFile = destFolder + os.path.basename(os.path.normpath(srcFile))
				paths.file_delete(destFile);
				
				copyCmd = 'cp -fR "' + srcFile + '" "' + destFolder + '"'
				os.system(copyCmd)
			else:
				if (srcFile[-1] == '/'):
					paths.copy_folder(srcFile, destFolder)
				else:
					paths.copy_folder(srcFile, destFolder + '/' + fileName)
			
			if fileName == 'Localize':
				rename_hungarian(destFolder)
		else:
			#debug_print("srcFile:    " + srcFile)
			#debug_print("destFolder: " + destFolder)
			shutil.copy(srcFile, destFolder)

def get_win_compiler_vers(bitDepth):
	vers = 'msvc' + paths.get_vs_year()

	if bitDepth == '64':
		vers += '_' + bitDepth
	
	return vers		
	
def copy_qt_runtimes(appDir, buildType, bitDepth, projName, in_srcFiles):
	sdkDir = paths.get_qt_IDE_folder(True)
	sdkDir += get_win_compiler_vers(bitDepth) + '/bin/'
	
	debug_print(sdkDir)
	
	srcFiles = []
	for curFile in in_srcFiles:
		srcFiles += [sdkDir + curFile]

	copy_to_bundle(	\
		appDir, buildType, bitDepth, projName, 'Windows/', srcFiles)

def copy_vc_runtimes(appDir, buildType, bitDepth, projName):
	is_debugB		= paths.is_debug(buildType)
	vcRedistDir		= paths.get_program_files_dir() + 'Microsoft Visual Studio/' + paths.get_vs_year() + '/'
	
	#	this should just find it regardless of VS edition
	if False:
		vcRedistDir += 'Enterprise'
	else:
		vcRedistDir += 'Community'
	
	vcRedistDir		+= '/VC/Redist/MSVC/'
	vcRedistDir		+= paths.get_vcRedist()
	vcRedistDir		+= 'onecore/'
	
	if is_debugB:
		vcRedistDir += 'debug_nonredist/'
	
	vcRedistDir += xplat.bid_depth_to_arch(bitDepth) + '/'
	vcRedistDir += 'Microsoft.VC142.'
	
	if is_debugB:
		vcRedistDir += paths.convert_debug_case(buildType)

	vcRedistDir += 'CRT/'
	ucrtRelDir = 'ucrt/'
	
	if is_debugB:
		# Windows Kits/10/bin/<win_sdk_vers>/x<depth>/ucrt/ucrtbased.dll'
		ucrtDir = paths.get_win_sdk_path()
	else:
		# Windows Kits/10/Redist/ucrt/DLLs/x<depth>/ucrtbase.dll'
		ucrtDir = paths.get_win10_kit_path() + 'Redist/' + ucrtRelDir + 'DLLs/'
		
	ucrtDir += xplat.bid_depth_to_arch(bitDepth) + '/'

	if is_debugB:
		ucrtDir += ucrtRelDir
	
	debugSuffix = ""
	if is_debugB:
		debugSuffix = "d"
		
	# files into "Contents/Windows/" folder
	srcFiles = [
		vcRedistDir	+ 'msvcp140'		+ debugSuffix + '.dll',
		vcRedistDir	+ 'vcruntime140'	+ debugSuffix + '.dll',
		ucrtDir		+ 'ucrtbase'		+ debugSuffix + '.dll'
	]

	copy_to_bundle(	\
		appDir, buildType, bitDepth, projName, 'Windows/', srcFiles)
	
def delete_src_exe(buildFolderParent, projName, bitDepth, buildType):
	
	debug_print(
		"delete_src_exe: " + \
		projName + ', ' + \
		bitDepth + ', ' + \
		buildType)

	if xplat.is_mac():
		extension = '.app'
	else:
		extension = '.exe'

	src_exe	= buildFolderParent + get_src_exe(buildType, bitDepth, projName, extension)
	
	debug_print(src_exe)

	if not os.path.exists(src_exe):
		debug_print("doesn't exist")
	else:
	
		if os.path.isfile(src_exe):
			debug_print("deleting file")
			os.remove(src_exe)
		else:
			debug_print("deleting folder")
			shutil.rmtree(src_exe, True)

def qt_deploy(buildFolderParent, buildType, bitDepth, projName, libPathDir = ""):
	appPath = get_app_path(buildFolderParent, buildType, bitDepth, projName)
	
	qt_IDE_folder	= paths.get_qt_IDE_folder(True)

	if xplat.is_mac():
		# on mac, bundle has already been created, 
		# but it must be moved into place
		
		qtDeployExe	= qt_IDE_folder + 'clang_64/bin/macdeployqt'
		qtDestAppPath = appPath[:-1]
	else:
		qtDeployExe	= qt_IDE_folder + get_win_compiler_vers(bitDepth)
		qtDeployExe += '/bin/windeployqt.exe'
		qtDestAppPath = get_exe_path(appPath)

	cmd = [
		qtDeployExe, 
		qtDestAppPath]

	if xplat.is_mac():
		# this doesn't seem to help
		if (paths.is_debug(buildType)):
			cmd += ['-verbose=0']
			#cmd += ['-use-debug-libs']

		if libPathDir != "":
			# this doesn't seem to help
			#sqlPathDir = '/Volumes/Developer/depot/kJams/Development/_source/Resources/package/mac/libsql/'
			#cmd += ['-libpath=' + sqlPathDir]

			#print libPathDir
			cmd += ['-libpath=' + libPathDir]
			
	else:
		cmd += ['--compiler-runtime', '--verbose=0']

	libVers = os.path.basename(qt_IDE_folder[:-1])
		
	print '\tCopying Qt Libraries: ' + libVers + '...'

	debug_print('cmd: ' + str(cmd))
	
	if True:
		# to show less logging / errors
		xplat.pipe(cmd, False);
	else:
		# note if this  runs,  you may see these problems, which aren't real:
		'''
		WARNING: Plugin "libqsqlodbc.dylib" uses private API and is not Mac App store compliant.
		WARNING: Plugin "libqsqlpsql.dylib" uses private API and is not Mac App store compliant.
		ERROR: no file at "/usr/local/opt/libiodbc/lib/libiodbc.2.dylib"
		ERROR: no file at "/Applications/Postgres.app/Contents/Versions/9.6/lib/libpq.5.dylib"
		'''
		# shows more errors
		xplat.system(cmd)
		pass

def copy_paddle(buildFolderParent, buildType, bitDepth, appName):
	kJamsDir = os.path.abspath(paths.get_script_folder(__file__) + '../../kJams') + '/'

	debug_print('----- COPY PADDLE -----')
	debug_print('kJamsDir: ' + kJamsDir)
	debug_print('buildFolderParent: ' + buildFolderParent)

	paddleDir = kJamsDir + 'External/Paddle/'

	if xplat.is_mac():
		paddleDir	+= 'Mac/framework/'
	
		srcFiles = [
			paddleDir + 'Paddle.framework',
		]

		bundleFolder = 'Frameworks/'
		
	else:
		is_qtB	= os.path.basename(os.path.normpath(buildFolderParent)) == 'qt'

		if is_qtB:
			# 32 or 64 bit
			wrapperDir	= paddleDir + '/Win/PaddleCPP-Windows/PaddleExample/PaddleExample/bin/'
			
			if bitDepth == '32':
				wrapperDir += 'x86/'
			else:
				wrapperDir += 'x64/'
		else:
			# 32 bit
			wrapperDir	= 'src/Paddle/bin/'
		
		debug_print("wrapperDir: " + wrapperDir);
		
		wrapperDir += paths.convert_debug_case(buildType) + '/'
	
		srcFiles = [
			wrapperDir	+ 'CredentialManagement.dll',
			wrapperDir	+ 'Interop.SHDocVw.dll',
			wrapperDir	+ 'Newtonsoft.Json.dll',
			wrapperDir	+ 'PaddleCLR.dll',
			wrapperDir	+ 'PaddleSDK.dll',
			wrapperDir	+ 'PaddleWrapper.dll',
			wrapperDir	+ 'StructureMap.dll',
			#wrapperDir	+ 'System.dll',
			#wrapperDir	+ 'System.Threading.dll',
		]

		'''
			if debugB: 
				srcFiles += [
					wrapperDir	+ 'PaddleCLR.pdb',
					wrapperDir	+ 'PaddleWrapper.pdb',
				]
		'''
		
		bundleFolder = 'Windows/'

	copy_to_bundle(									\
		buildFolderParent, buildType, bitDepth, 	\
		appName, bundleFolder, srcFiles)

#-----------------------------------------------------
# args: 0:QT/VS, 1:debug/release, 2:32/64, 3:src_exePath, 4:appPath
# test: python post_build_cf.py QT debug 64 "" ~/Desktop/kJams\ Pro\ 64.app
if __name__ == "__main__":
	args = sys.argv[1:]
	
	if len(args) != 4:
		print 'CoreFoundation configs not all specified.'
	else:
		create_bundle(args[0] == "QT", args[1], args[2], args[3], args[4])
