#ifndef MWA_FITS_H
#define MWA_FITS_H

#include <string>

/**
 * This module writes the extended MWA keywords into uvfits files
 */

struct MWAFitsEnums
{
	enum MWAKeywords
	{
		//MWA_FIBER_VEL_FACTOR,
		MWA_METADATA_VERSION,
		MWA_MWAPY_VERSION,
		MWA_COTTER_VERSION, MWA_COTTER_VERSION_DATE
	};
};

class MWAFits
{
	public:
		MWAFits(const std::string &filename);
		
		~MWAFits();
		
		void WriteMWAKeywords(const std::string& metaDataVersion, const std::string& mwaPyVersion, const std::string& cotterVersion, const std::string& cotterVersionDate);
		
	private:
		static const char* keywordName(enum MWAFitsEnums::MWAKeywords keyword)
		{
			return _keywordNames[(int) keyword];
		}
		static const char* keywordComment(enum MWAFitsEnums::MWAKeywords keyword)
		{
			return _keywordComments[(int) keyword];
		}
		template<typename T>
		void writeKey(enum MWAFitsEnums::MWAKeywords keyword, const T& value);
		
		static const char* _keywordNames[];
		static const char* _keywordComments[];
		
		const std::string _filename;
		struct MWAFitsData *_data;
};

#endif
