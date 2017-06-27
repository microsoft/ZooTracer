#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <map>

namespace zt {

#define DECLARE_READ_WRITE_FUNCTIONS(T) \
	void file_read(T &, FILE *);        \
	void file_write(T const &, FILE *); \
	std::string get_name_of(T const &);


	DECLARE_READ_WRITE_FUNCTIONS(bool)
	DECLARE_READ_WRITE_FUNCTIONS(char)
	DECLARE_READ_WRITE_FUNCTIONS(unsigned char)
	DECLARE_READ_WRITE_FUNCTIONS(int)
//	DECLARE_READ_WRITE_FUNCTIONS(uint)
	DECLARE_READ_WRITE_FUNCTIONS(float)
	DECLARE_READ_WRITE_FUNCTIONS(double)
	DECLARE_READ_WRITE_FUNCTIONS(size_t)

	void file_write(std::string const & s, FILE * f);
	void file_read(std::string & s, FILE * f);

	template <class T> void file_read(T & d, FILE * f){ d.saveTo(f); }
	template <class T> void file_write(T const & d, FILE * f){ d.loadFrom(f); }
	template <class T> std::string get_name_of(T const & d){ return d.name(); }

	//template <class T> void file_read(std::shared_ptr<T> & d, FILE * f);
	//template <class T> void file_write(std::shared_ptr<T> const & d, FILE * f);

	template <class T> void file_read(std::vector<T> & v, FILE * f){
		int count = 0;
		file_read(count, f);
		v.resize(count);
		for (int i = 0; i < count; ++i)
		{
			file_read(v[i], f);
		}
	}

	template <class T> void file_write(std::vector<T> const & v, FILE * f){
		file_write(static_cast<int>(v.size()), f);
		for (std::vector<T>::const_iterator i = v.begin(); i != v.end(); ++i)
		{
			file_write(*i, f);
		}
	}

	template <class U, class V> void file_read(std::map<U, V> & m, FILE * target){
		m.clear();
		int count = 0;
		file_read(count, f);
		for (int i = 0; i < count; ++i)
		{
			U first;
			file_read(first, f);
			file_read(m[first], f);
		}
	}

	template <class U, class V> void file_write(std::map<U, V> const & m, FILE * target){
		file_write(static_cast<int>(m.size()), f);
		std::map<U, V>::const_iterator it = m.begin();
		for (; it != m.end(); ++it)
		{
			file_write(it->first, f);
			file_write(it->second, f);
		}
	}

	void saveName(std::string const & name, FILE * target);
	void checkName(std::string const & name, FILE * source);

	template <class T> std::string get_name_of(std::vector<T> const & v){
		return v.size() == 0 ? "" : get_name_of(c[0]) + "Vector";
	}

	template <class U, class V> std::string get_name_of(std::map<U, V> const &){
		std::vector<U> a(1);
		std::string name1 = get_name_of(a[0]);
		std::vector<V> b(1);
		std::string name2 = get_name_of(b[0]);
		return "Map(" + name1 + "->" + name2 + ")";
	}

	//template <class T> void save_to_file(T const &, std::string const & filename);
	//template <class T> void load_from_file(T &, std::string const & filename);

	class Saveable
	{
	protected:
		void saveName(FILE * target) const;
		void checkName(FILE * source);

	public:
		Saveable() {}
		~Saveable() {}

		void saveToFile(std::string const & filename) const;
		void loadFromFile(std::string const & filename);

		virtual std::string name() const = 0;

		virtual void saveTo(FILE *) const = 0;
		virtual void loadFrom(FILE *) = 0;
	};
}



//template <class T>
//void AmpTrack::file_write(std::shared_ptr<T> const & d, FILE * f)
//{
//	file_write( *d , f);
//}
//template <class T>
//void AmpTrack::file_read(std::shared_ptr<T> & d, FILE * f)
//{
//	d.reset( new T() );
//	file_read( *d, f );
//}




//template <class T>
//void AmpTrack::save_to_file(T const & object, std::string const & filename)
//{
//	FILE * target;
//	errno_t err = fopen_s(&target, filename.c_str(), "wb");
//	if (err) throw error("Couldn't open '" + filename + "' for writing.");
//	// this will double up in many situations, but this is fine
//	saveName(get_name_of(object), target);
//	file_write(object, target);
//	fclose(target);
//}
//
//template <class T>
//void AmpTrack::load_from_file(T & object, std::string const & filename)
//{
//	FILE * source;
//	errno_t err = fopen_s(&source, filename.c_str(), "rb");
//	if (err) throw error("Couldn't open '" + filename + "' for reading.");
//	try { checkName(get_name_of(object), source); }
//	catch (std::exception e) { printf("%s\n", e.what()); abort(); } // DEBUG // TODO!: remove!
//	catch (...) { throw; }
//	file_read(object, source);
//	fclose(source);
//}