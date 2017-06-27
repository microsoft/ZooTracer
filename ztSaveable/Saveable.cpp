#include <ztSaveable.h>
#include <ztLog.h>

using namespace zt;

#define DEFINE_READ_WRITE_FUNCTIONS(T)                                                  \
	std::string zt::get_name_of(T const &) { return #T; }                        \
	void zt::file_read(T & d, FILE * f) { fread(&d, sizeof(T), 1, f); }         \
	void zt::file_write(T const & d, FILE * f) { fwrite(&d, sizeof(T), 1, f); }

DEFINE_READ_WRITE_FUNCTIONS(bool)
DEFINE_READ_WRITE_FUNCTIONS(char)
DEFINE_READ_WRITE_FUNCTIONS(unsigned char)
DEFINE_READ_WRITE_FUNCTIONS(int)
//DEFINE_READ_WRITE_FUNCTIONS(uint)
DEFINE_READ_WRITE_FUNCTIONS(float)
DEFINE_READ_WRITE_FUNCTIONS(double)
// std::vector<bool> non-comformity strikes again!
//std::string zt::get_name_of(std::_Vb_reference<std::allocator<bool>> const &) { return "bool"; }

void zt::file_write(std::string const & s, FILE * f)
{
	// fgets uses only '\n' (newline) as delimiter, 
	fputs(s.c_str(), f);
	fputc('\n', f);
}

void zt::file_read(std::string & s, FILE * f)
{
	char buffer[1024];
	fgets(buffer, 1024, f);
	buffer[strlen(buffer) - 1] = 0; // erase newline charater
	s.assign(buffer);
}

void zt::saveName(std::string const & name, FILE * target)
{
	file_write(name, target);
}

void zt::checkName(std::string const & this_name, FILE * source)
{
	// TODO: pseudo backwards compatibility by stepping through until name found
	std::string read_name;
	file_read(read_name, source);
	if (read_name != this_name)
		throw std::exception(("Wrong name in file:  expected '" + this_name + "', found '" + read_name + "'").c_str());
}

void Saveable::saveName(FILE * target) const
{
	zt::saveName(name(), target);
}

void Saveable::checkName(FILE * source)
{
	try { zt::checkName(name(), source); }
	catch (std::exception e) { printf("%s\n", e.what()); abort(); } // DEBUG // TODO!: remove!
	catch (...) { throw; }
}

void Saveable::saveToFile(std::string const & filename) const
{
	FILE * target;
	errno_t err = fopen_s(&target, filename.c_str(), "wb");
	if (err) throw std::exception(("Couldn't open '" + filename + "' for writing.").c_str());
	// this will double up in many situations, but this is fine
	this->saveName(target);
	this->saveTo(target);
	fclose(target);
}

void Saveable::loadFromFile(std::string const & filename)
{
	FILE * source;
	errno_t err = fopen_s(&source, filename.c_str(), "rb");
	if (err) throw std::exception(("Couldn't open '" + filename + "' for reading.").c_str());
	try
	{
		this->checkName(source);
		this->loadFrom(source); // IGD moved - makes more sense here.
	}
	catch (std::exception e) { 
		Log::write(e.what());
		throw;
	}
	fclose(source);
}

