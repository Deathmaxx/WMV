#ifndef DBFILE_H
#define DBFILE_H

#include <cassert>
#include <map>
#include <string>
#include <vector>

#include <QString>

#include "GameDatabase.h"

#ifdef _WIN32
#    ifdef BUILDING_CORE_DLL
#        define _DBFILE_API_ __declspec(dllexport)
#    else
#        define _DBFILE_API_ __declspec(dllimport)
#    endif
#else
#    define _DBFILE_API_
#endif


class _DBFILE_API_ DBFile
{
public:
  explicit DBFile();
  virtual ~DBFile() {};

	virtual bool open() = 0;

  virtual bool close() = 0;

	// Iteration over database
	class Iterator
	{
	  public:
    Iterator(DBFile &f, unsigned int index) :
      file(f), recordIndex(index) {}
		  
      /// Advance (prefix only)
		  Iterator & operator++() 
      { 
        recordIndex++;
			  return *this; 
		  }	
		
      std::vector<std::string> get(const core::TableStructure * structure) const
      {
        return file.get(recordIndex, structure);
      }
	
		  /// Comparison
		  bool operator==(const Iterator &b) const
		  {
        return recordIndex == b.recordIndex;
		  }
		
      bool operator!=(const Iterator &b) const
		  {
        return recordIndex != b.recordIndex;
		  }
	  
    private:
      DBFile &file;
      unsigned int recordIndex;
	};

	/// Get begin iterator over records
	Iterator begin();
	/// Get begin iterator over records
	Iterator end();

	/// Trivial
	size_t getRecordCount() const { return recordCount; }

  // to be implemented in inherited classes to get actual record values (specified by recordOffset), following "structure" format
  virtual std::vector<std::string> get(unsigned int recordIndex, const core::TableStructure * structure) const = 0;

protected:
  uint32_t recordSize;
  uint32_t recordCount;
  uint32_t fieldCount;
  uint32_t stringSize;
	unsigned char *data;
	unsigned char *stringTable;

  private:
  DBFile(const DBFile &);
  void operator=(const DBFile &);
};

#endif
