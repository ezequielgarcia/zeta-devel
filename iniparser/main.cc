
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

#define INIFILENAME "something.ini"

// FIXME: Revisar el tabulado

class Section {

private:
	string _name;
	map<string,string> _key_value;

public:

	Section(string& name) {
		_name = name;
	}

	string name() {
		return _name;
	}

	void insert(string& key, string& value) {
		_key_value.insert(make_pair(key,value));
	}

	map<string,string>& get_map() {
		return _key_value;
	}
};

static
void trim(string& str)
{
	size_t pos1 = str.find_first_not_of(" ");

//	str = str.substr(	pos1 == string::npos ? 0 : pos1, 
//			pos2 == string::npos ? str.length() - 1 : pos2 - pos1 + 1);
	
	if (pos1 != string::npos)
		str.erase(0, pos1);

	size_t pos2 = str.find_last_not_of(" ");

	if (pos2 != string::npos)
		str.erase(pos2+1);
}

int main(int argc, char* argv[])
{
	string file_name;
	string line;
	vector<Section*> vec;
	map<string, Section*> m;

	// Nombre de archivo
	if (argc > 1) 
		file_name = string(argv[1]);
	else 
		file_name = INIFILENAME;

	// Leer archivo
	ifstream ifs(file_name.c_str());

	// Recorrerlo linea por linea hasta la primer seccion
	while( getline( ifs, line ) ) {
	
		// Trim leading, ending whitespaces
		trim(line);

		// Buscamos secciones: lineas que empiecen con '['
		if (line[0] == '[') 
			break;
	}
	
	if ( line.empty() || line[0] != '[' ) {
		cout << "No sections in file!" << endl;
		return 0;
	}

	Section* current = NULL;
	do {

		// Trim leading, ending whitespaces
		trim(line);

		if (line.empty() || line[0] == ';')
			continue;

		if (line[0] == '[') {

			// New section; push old one
			if (current != NULL)
				m.insert(make_pair(current->name(), current));

			string name = line.substr(1,line.find_first_of(']')-1);

			current = new Section(name);	
		}
		else {

			if (!isalnum(line[0])) {
				cout << "Malformed section: key start is not alphanumeric" << endl;
				break;
			}

			size_t j = line.find_first_of("=");
			size_t k = line.find_first_of(";");

			if (k<j) {
				cout << "Malformed section: '=' not found" << endl;
				break;
			}

			string key = line.substr(0, j-1);
			string value = line.substr(j+1, k-1);

			// Trim leading, ending whitespaces
			trim(key);
			trim(value);

			current->insert(key,value);
		}

	} while( getline( ifs, line ) );

	// push remaining one
	if (current != NULL)
		m.insert(make_pair(current->name(), current));

	// show content:
	map<string,Section*>::iterator it;
	for ( it=m.begin() ; it != m.end(); it++ ) {

		map<string,string> mm = it->second->get_map();	
		map<string,string>::iterator itt;

		cout << "-- " << it->first << endl;

		for ( itt=mm.begin() ; itt != mm.end(); itt++ ) 
			cout << "[" << (*itt).first << "]" << " => " << "[" << (*itt).second << "]" << endl;
	}
	
	return 0;
}
