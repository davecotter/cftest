import sys, shutil, os, stat, glob, platform
import include
import post_build_cf, paths, xplat

def debug_print(str):
	#print "post_build_cftest: " + str
	pass

#-----------------------------------------------------

def main(qtB, bitDepth, buildType, ssl_edition):

	projName		= 'CFTest'
	
	debug_print('buildType: ' + buildType)
	
	if paths.is_debug(buildType):
		projName += ' ' + paths.convert_debug_case(buildType)
	
	scriptDir		= paths.get_script_folder(__file__)

	if (qtB):
		projDir		= scriptDir + 'qt/'
		debug_print("projDir: " + projDir)
		
		debug_print("Creating QT bundle")

		if xplat.is_mac():
			# on mac, bundle has already been created, 
			# but it must be moved into place
			
			debug_print("buildType: " + buildType);
			debug_print("projName: " + projName);
			appPath = post_build_cf.get_app_path(projDir, buildType, bitDepth, projName)
	
			# pop trailing slash
			appPath = appPath[:-1]
			debug_print("appPath: " + appPath);

			src_exe	= projDir + post_build_cf.get_src_exe(buildType, bitDepth, projName, '.app')
			debug_print("src_exe: " + src_exe)
			debug_print("appPath: " + appPath)
		
			if os.path.exists(appPath):
				debug_print('deleting previous')
				shutil.rmtree(appPath, True)

			print '	creating bundle...'
			paths.copy_folder(src_exe, appPath)
		else:
			post_build_cf.create_bundle_qt(projDir, buildType, bitDepth, projName, ssl_edition)
			
			if paths.is_debug(buildType):
				pass
				# no need to copy cuz it's debug?
				#post_build_cf.qt_deploy(projDir, buildType, bitDepth, projName)
				#post_build_cf.copy_vc_runtimes(projDir, buildType, bitDepth, projName)
			else:
				pass
				# no need cuz it doesn't USE any QT libs?
				#post_build_cf.qt_deploy(projDir, buildType, bitDepth, projName)
				#post_build_cf.copy_vc_runtimes(projDir, buildType, bitDepth, projName)

	else:
		debug_print("Creating VS bundle")

		buildType = paths.convert_debug_case(buildType)

		projDir = scriptDir + 'win_vs/'
		
		dest_appPath	= projDir + 'build/' + projName + '.app/';
		debug_print("dest_appPath: " + dest_appPath)

		src_exe			= projDir + buildType + '/' + projName + '.exe'
		debug_print("src_exe: " + src_exe)

		# create bundle, copy CF stuff
		post_build_cf.create_bundle(False, buildType, bitDepth, src_exe, dest_appPath, ssl_edition)

#-----------------------------------------------------
# args:
# 0: QT/VS
# 1: 32/64
# 2: CFTest (ignored)
# 3: Debug (optional, default is "" which means "Release")
# 4: openssl/libressl (optional, default is "libressl")

if __name__ == "__main__":
	args = sys.argv[1:]
	
	lenI = len(args)
	
	if lenI < 3 or lenI > 5:
		print 'CFTest configs not properly specified.'

		print "args: " + str(lenI)
		for i in xrange(lenI):
			print str(i) + ": " + args[i]
	else:
		qtB = args[0] == "QT"

		# args[2] is ignored

		buildType	= 'release'
		ssl_edition	= 'libressl'
		
		if (lenI > 3):
			nextArg = 3
			
			if paths.is_debug(args[3]):
				buildType = 'debug'
				nextArg = 4
			
			if (lenI > nextArg):
				if args[nextArg] != ssl_edition:
					ssl_edition = args[nextArg]

		'''
		debug_print("args: "	\
			+ ', ' + args[0]	\
			+ ', ' + args[1]	\
			+ ', ' + buildType	\
			+ ', ' + ssl_edition)
		'''
			
		main(qtB, args[1], buildType, ssl_edition)
