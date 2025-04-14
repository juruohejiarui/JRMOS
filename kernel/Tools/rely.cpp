// find all needed *.c and *.S files excepts those in Tools/
// use gcc -MM for getting all the .h file that this file relies on.
// then output to .depend file
#include <string>
#include <cstring>
#include <iostream>
#include <dirent.h>
#include <format>
#include <vector>

std::vector<std::string> objList;
std::vector<std::string> tmpList;
std::vector<std::string> excludeList;
std::string archName;
std::string cplName;
std::string flagName;
std::string outputSuf;
std::string rootPath;
std::string relyPath;
std::string objListName;
std::string outListName;

void searchFile(const std::string &path) {
	for (size_t i = 0; i < excludeList.size(); i++) if (excludeList[i] == path) return ;
	DIR *dir = opendir(path.c_str());
	dirent *ptr;
	if (dir == NULL) {
		printf("failed to open directory %s\n", path.c_str());
		return ;
	}
	while ((ptr = readdir(dir)) != NULL) {
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		else if (ptr->d_type == 0x8) {
			// printf("file     : %s/%s\n", path.c_str(), ptr->d_name);
			std::string filePath = path + "/" + ptr->d_name;
			std::string objPath = filePath;
			std::string outPath = filePath;
			if (outPath[outPath.size() - 2] != '.') continue;
			objPath[objPath.size() - 1] = 'o';
			outPath = outPath.substr(0, outPath.size() - 1).append(outputSuf);
			std::string suf = filePath.substr(filePath.size() - 2);
			if (suf == ".c") {
				system(std::format("echo -n \"{0}/\" >> {1}", path, relyPath).c_str());
				std::string cmd = std::format("gcc -MM {0} -I./includes/ | tee -a {1} > /dev/null", filePath, relyPath);
				system(cmd.c_str());
				system(std::format(
					"echo \"\\t@\\$({3}) \\$({4}) -c {0} -o {1}\\n"
					"\\t@echo \\\"{3:8} {0}\\\"\" >> {2}", filePath, objPath, relyPath, cplName, flagName).c_str());
				if (outputSuf != "o") {
					system(std::format(
						"echo \"{0}: {1}\\n"
						"\\t@\\$({2}) {1} -o {0}\\n"
						"\\t@\\{0}\\n"
						"\\t@echo \\\"GEN      {1}\\\"\" >> {3}", outPath, objPath, cplName, relyPath).c_str()
					);
					tmpList.push_back(outPath);
				}
				
				objList.push_back(objPath);
			} else if (suf == ".S") {
				std::string tmpPath = filePath;
				tmpPath.append("TMP");
				system(std::format("echo -n \"{0}/\" >> {1}", path, relyPath).c_str());
				std::string cmd = std::format("gcc -MM {0} -I./includes/ | tee -a {1} > /dev/null", filePath, relyPath);
				system(cmd.c_str());
				system(std::format(
					"echo \"\\t@\\$({4}) -E \\$(C_INCLUDE_PATH) {0} > {1}\\n"
					"\\t@\\$(ASM) \\$(ASM_FLAGS) -o {2} {1}\\n"
					"\\t@echo \\\"ASM      {0}\\\"\" >> {3}", filePath, tmpPath, objPath, relyPath, cplName).c_str());

				objList.push_back(objPath);
				tmpList.push_back(tmpPath);
			}
			
		} else if (ptr->d_type == 0x10) {
			// printf("link file: %s/%s\n", path.c_str(), ptr->d_name);
		} else {
			std::string subPath = path;
			subPath.push_back('/');
			subPath.append(ptr->d_name);
			if ((path == "./hal" && subPath != "./hal/" + archName)) continue;
			bool isExclude = false;
			for (size_t i = 0; i < excludeList.size(); i++) if (excludeList[i] == ptr->d_name) {
				isExclude = true;
				break;
			}
			if (!isExclude) searchFile(subPath);
		}
	}
}

int main(int argc, char **argv) {
	if (argc < 9) {
		printf("Error: format: [arch_name] [compiler] [flag_name] [output_suffix] [rely_path] [root_path] [obj_list_name] [out_list_name] (-exclude path 1) ... ");
		return -1;
	}
	archName = std::string(argv[1]);
	cplName = std::string(argv[2]);
	flagName = std::string(argv[3]);
	outputSuf = std::string(argv[4]);
	relyPath = std::string(argv[5]);
	rootPath = std::string(argv[6]);
	objListName = std::string(argv[7]);
	outListName = std::string(argv[8]);
	for (int i = 9; i < argc; i++) excludeList.push_back(argv[i]);
	system("rm includes/hal");
	system(std::format("ln -s ../hal/{0}/includes includes/hal", archName).c_str());
	system(std::format("echo \"\" > {0}", relyPath).c_str());
	searchFile(rootPath);
	std::string allObj = std::format("{0}=", objListName);
	for (const auto &obj : objList) allObj.append(obj + " ");
	system(std::format("echo \"{0}\" >> {1}", allObj, relyPath).c_str());
	std::string allTmp = std::format("{0}= ", outListName, objListName);
	for (const auto &tmp : tmpList) allTmp.append(tmp + " ");
	system(std::format("echo \"{0}\" >> {1}", allTmp, relyPath).c_str());
	return 0;
}