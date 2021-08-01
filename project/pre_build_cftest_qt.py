import sys, os, shutil, stat, glob, platform
import include
import post_build_cf, paths, xplat

def debug_print(str):
	#xplat.log_message("pre_build_cftest_qt: " + str)
	pass

#-----------------------------------------------------
# args: 
# 0: buildKit
# 1: CFTest
# 2: Debug / Release (optional, default = Release)
#
'''
	post_build_cf: delete_src_exe: qtDir, CFTest, 64, Debug
	post_build_cf: params: 
		 buildType: debug
		 buildKit: qt5_orig
		 projName: CFTest
		 buildName: CFTest
'''
if __name__ == "__main__":
	if True:
		args = sys.argv[1:]
	
		lenI = len(args)
	
		if lenI != 3 and lenI != 2:
			xplat.log_message('CFTest Pre Build args not correctly specified.')
		else:
			qtDir	= paths.get_script_folder(__file__) + 'qt/'
			debug_print("qtDir: " + qtDir)
			
			if lenI == 3:
				buildType = args[2]
			else:
				buildType = "Release"
				
			projName = args[1]

			if buildType == 'Debug':
				projName += ' ' + buildType

			debug_print("----- start")
			post_build_cf.delete_src_exe(qtDir, projName, args[0], buildType)
			debug_print("----- end")
