/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                                 *
 * Copyright (c) 2019, William C. Lenthe                                           *
 * All rights reserved.                                                            *
 *                                                                                 *
 * Redistribution and use in source and binary forms, with or without              *
 * modification, are permitted provided that the following conditions are met:     *
 *                                                                                 *
 * 1. Redistributions of source code must retain the above copyright notice, this  *
 *    list of conditions and the following disclaimer.                             *
 *                                                                                 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,    *
 *    this list of conditions and the following disclaimer in the documentation    *
 *    and/or other materials provided with the distribution.                       *
 *                                                                                 *
 * 3. Neither the name of the copyright holder nor the names of its                *
 *    contributors may be used to endorse or promote products derived from         *
 *    this software without specific prior written permission.                     *
 *                                                                                 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"     *
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE       *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE  *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE    *
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL      *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR      *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   *
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.            *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef _tsl_h_
#define _tsl_h_

#include <vector>
#include <string>
#include <cstring>
#include <cctype>
#include <sstream>
#include <cstdint>
#include <fstream>
#include <cmath>
#include <algorithm>

#include "mmap.hpp"

namespace tsl {
	struct HKLFamily {
		int32_t hkl[3]   ;//hkl plane
		int32_t useIdx   ;//use in indexing
		int32_t intensity;//diffraction intensity
		int32_t showBands;//overlay bands on indexed patterns
	};

	struct Phase {
		size_t                 num   ;//phase number
		std::string            name  ;//material name
		std::string            form  ;//chemical formula
		std::string            info  ;//additional information
		uint32_t               sym   ;//tsl symmetry number
		float                  lat[6];//lattice constants (a, b, c, alpha, beta, gamma)
		std::vector<HKLFamily> hklFam;//hkl families
		float                  el[36];//elastic constants (6x6 matrix in row major order)
		std::vector<size_t>    cats  ;//categories (not sure what this is)
	};

	//enumeration of grid types
	enum class GridType {Unknown, Square, Hexagonal};

	//@brief: read a grid type from an input stream
	//@param is: stream to read from
	//@param grid: grid type to read into
	//@return: stream read from
	std::istream& operator>>(std::istream& is,       GridType& grid);

	//@brief: write a grid to an output stream
	//@param 0s: stream to write to
	//@param grid: grid type to write from
	//@return: stream written to
	std::ostream& operator<<(std::ostream& os, const GridType& grid);

	//enumeration of file types
	enum class FileType {Unknown, Ang, Osc, Hdf};

	//@brief: get the type of a file
	//@param fileName: name to parse extension from
	//@return: parsed extension
	FileType getFileType(std::string fileName);

	class OrientationMap {
		public:
			//header information
			float  pixPerUm             ;
			float  xStar, yStar, zStar  ;//pattern center calibration
			float  workingDistance      ;//working distance in mm
			float  xStep   , yStep      ;//pixel size in microns
			size_t nColsOdd, nColsEven  ;//width in pixels (same for square grid, alternating rows for hex grid)
			size_t nRows                ;//height in pixels
			std::string operatorName    ;//operator name
			std::string sampleId        ;//same ID string
			std::string scanId          ;//scan ID string
			GridType    gridType        ;//square/hex grid
			std::vector<Phase> phaseList;//list of indexed phases (the 'phase' scan data indexes into this array)

			//scan data (all in row major order)
			std::vector<float > eu   ;//euler angle triples for each pixel
			std::vector<float > x, y ;//x/y coordinate of pixel in microns
			std::vector<float > iq   ;//image quality
			std::vector<float > ci   ;//confidence index
			std::vector<float > sem  ;//secondary electron signal
			std::vector<float > fit  ;//fit
			std::vector<size_t> phase;//phase ID of each pixel (indexes into phaseList)

			//@brief: construct an empty orientation map
			OrientationMap() : gridType(GridType::Unknown) {}

			//@brief: construct an orientation map from a file
			//@param fileName: file to read (currently only .ang is supported)
			OrientationMap(std::string fileName) : gridType(GridType::Unknown) {read(fileName);}

			//@brief: check if a file can be ready by this class (based on file extension)
			//@return: true/false if the file type can/cannot be read
			static bool CanRead(std::string fileName) {return FileType::Ang == getFileType(fileName);}//currently only ang reading is implemented here

			//@brief: allocate space to hold scan data based on grid type and dimensions
			//@param tokenCount: number of arrays to use - eu, x, y, iq, ci and phase are always allocated, sem is only allocated for 9+ tokens and fit for 10+
			void allocate(const size_t tokenCount);

			//@brief: read scan data from a TSL orientation map file
			//@param fileName: file to read (currently only .ang is supported)
			void read(std::string fileName);

			// //@brief: write scan data to a 
			// //@param fileName: file to read (currently only .ang is supported)
			// void read(std::string fileName);

		private:
			//@brief: read data from a '.ang' file
			//@param fileName: name of ang file to read
			//@return: number of scan points read from file
			size_t readAng(std::string fileName);

			//@brief: read an ang header and parse the values
			//@param is: input stream to read the header from
			//@return: number of tokens (number of data columns)
			size_t readAngHeader(std::istream& is);

			//@brief: read ang data using an input stream
			//@param is: input stream set data start
			//@param tokens: number of tokens per point
			//@return: number of points (rows) parsed
			size_t readAngData(std::istream& is, size_t tokens);

			//@brief: read ang data using a memory map
			//@param fileName: name of file to read
			//@param offset: offset to data start (in bytes)
			//@param tokens: number of tokens per point
			//@return: number of points (rows) parsed
			size_t readAngDataMemMap(std::string fileName, std::streamoff offset, size_t tokens);
	};

	////////////////////////////////////////////////////////////////////////////////
	//                           Implementation Details                           //
	////////////////////////////////////////////////////////////////////////////////

	//@brief: read a GridType from an input stream
	//@param is: input stream to read from
	//@param grid: location to write parsed grid type
	//@return: input stream read from
	std::istream& operator>>(std::istream& is, GridType& grid) {
		std::string name;
		is >> name;
		if     (0 == name.compare("SqrGrid")) grid = GridType::Square;
		else if(0 == name.compare("HexGrid")) grid = GridType::Hexagonal;
		else grid = GridType::Unknown;
		return is;
	}

	//@brief: read a GridType from an output stream
	//@param os: output stream to write to
	//@param grid: grid type to write
	//@return: output stream written to
	std::ostream& operator<<(std::ostream& os, const GridType& grid) {
		switch(grid) {
			case GridType::Square   : return os << "SqrGrid";
			case GridType::Hexagonal: return os << "HexGrid";
			default: throw std::runtime_error("unknown grid type");
		}
	}

	//@brief: get the type of a file
	//@param fileName: name to parse extension from
	//@return: parsed extension
	FileType getFileType(std::string fileName) {
		size_t pos = fileName.find_last_of(".");//find the last '.' in the name
		if(std::string::npos == pos) return FileType::Unknown;//handle files with no extension
		std::string ext = fileName.substr(pos+1);//extract the file extension
		std::transform(ext.begin(), ext.end(), ext.begin(), [](char c){return std::tolower(c);});//convert to lowercase

		if     (0 == ext.compare("ang" )) return FileType::Ang;
		else if(0 == ext.compare("osc" )) return FileType::Osc;
		else if(0 == ext.compare("hdf" )) return FileType::Hdf;
		else if(0 == ext.compare("hdf5")) return FileType::Hdf;
		else if(0 == ext.compare("h5"  )) return FileType::Hdf;
		else return FileType::Unknown;
	}

	//@brief: allocate space to hold scan data based on grid type and dimensions
	//@param tokenCount: number of arrays to use - eu, x, y, iq, ci and phase are always allocated, sem is only allocated for 9+ tokens and fit for 10+
	void OrientationMap::allocate(const size_t tokenCount) {
		//compute number of pixels based on dimensions and grid type
		size_t totalPoints = 0;
		switch(gridType) {
			case GridType::Square: {
				totalPoints = std::max(nColsOdd, nColsEven) * nRows;
			} break;

			case GridType::Hexagonal: {
				totalPoints = size_t(nRows / 2) * (nColsOdd + nColsEven);
				if(1 == nRows % 2) totalPoints += nColsOdd;
			} break;

			default: throw std::runtime_error("only Square and Hexagonal grid types are supported");
		}

		//allocate arrays (filling new space with zero)
		eu   .resize(3*totalPoints);
		x    .resize(  totalPoints);
		y    .resize(  totalPoints);
		iq   .resize(  totalPoints);
		ci   .resize(  totalPoints);
		phase.resize(  totalPoints);
		if(tokenCount > 8) sem.resize(totalPoints);
		if(tokenCount > 9) fit.resize(totalPoints);
	}

	//@brief: construct an orientation map from a file
	//@param fileName: file to read (currently only .ang is supported)
	void OrientationMap::read(std::string fileName) {
		//read data from the file
		size_t pointsRead = 0;//nothing has been read
		switch(getFileType(fileName)) {//dispatch the file to the appropraite reader based on the extension
			case FileType::Ang: pointsRead = readAng(fileName);;
			default: std::runtime_error("unsupported file type (currently only .ang files are supported)");
		}

		//check that enough data was read
		if(pointsRead < iq.size()) {//I'll compare against IQ since it is always present (and easier than comparing against euler angles)
			std::stringstream ss;
			ss << "file ended after reading " << pointsRead << " of " << iq.size() << " data points";
			throw std::runtime_error(ss.str());
		}
	}

	//@brief: read data from a '.ang' file
	//@param fileName: name of ang file to read
	//@return: number of scan points read from file
	size_t OrientationMap::readAng(std::string fileName) {
		//parse the header
		std::ifstream is(fileName.c_str());//open file
		if(!is) throw std::runtime_error("ang file " + fileName + " doesn't exist");
		size_t tokenCount = readAngHeader(is);//read header and count number of tokens per point
		allocate(tokenCount);//allocate space

		//read the data
		static const bool UseMemMap = true;
		if(UseMemMap) {
			std::streamoff offset = is.tellg();//get offset to data start
			is.close();//close the file
			return readAngDataMemMap(fileName, offset, tokenCount);
		} else {
			return readAngData(is, tokenCount);
		}
	}

	//@brief: read an ang header and parse the values
	//@param is: input stream to read the header from
	//@return: number of tokens (number of data columns)
	size_t OrientationMap::readAngHeader(std::istream& is) {
		//flags for which header tokens have been parsed
		bool readPixPerUm        = false;
		bool readXStar           = false, readYStar    = false, readZStar = false;
		bool readWorkingDistance = false;
		bool readXStep           = false, readYStep    = false;
		bool readColsOdd         = false, readColsEven = false;
		bool readRows            = false;
		bool readOperatorName    = false;
		bool readSampleId        = false;
		bool readScanId          = false;
		bool readGridType        = false;

		//flags for which phase subheader tokens have been parsed (set to true values in case there are no phases listed)
		bool   readPhaseSymmetry = true;
		size_t targetFamilies    = 0   ;
		bool   readPhaseMaterial = true;
		bool   readPhaseFormula  = true;
		bool   readPhaseInfo     = true;
		bool   readPhaseLattice  = true;
		bool   readPhaseHkl      = true;
		size_t phaseElasticCount = 6   ;
		bool readPhaseCategories = true;

		//now start parsing the header
		char line[512];//buffer to hold single line of header
		std::string token;//current header token
		while('#' == is.peek()) {//all header lines start with #, keep going until we get to data
			is.getline(line, sizeof(line));//extract entire line from file
			std::istringstream iss(line+1);//skip the '#'
			if(iss >> token) {//get the key word if the line isn't blank
				//get value for appropriate key
				if       (0 == token.compare("TEM_PIXperUM"   )) {iss >> pixPerUm       ; readPixPerUm        = true;
				} else if(0 == token.compare("x-star"         )) {iss >> xStar          ; readXStar           = true;
				} else if(0 == token.compare("y-star"         )) {iss >> yStar          ; readYStar           = true;
				} else if(0 == token.compare("z-star"         )) {iss >> zStar          ; readZStar           = true;
				} else if(0 == token.compare("WorkingDistance")) {iss >> workingDistance; readWorkingDistance = true;
				} else if(0 == token.compare("GRID:"          )) {iss >> gridType       ; readGridType        = true;
				} else if(0 == token.compare("XSTEP:"         )) {iss >> xStep          ; readXStep           = true;
				} else if(0 == token.compare("YSTEP:"         )) {iss >> yStep          ; readYStep           = true;
				} else if(0 == token.compare("NCOLS_ODD:"     )) {iss >> nColsOdd       ; readColsOdd         = true;
				} else if(0 == token.compare("NCOLS_EVEN:"    )) {iss >> nColsEven      ; readColsEven        = true;
				} else if(0 == token.compare("NROWS:"         )) {iss >> nRows          ; readRows            = true;
				} else if(0 == token.compare("OPERATOR:"      )) {iss >> operatorName   ; readOperatorName    = true;
				} else if(0 == token.compare("SAMPLEID:"      )) {iss >> sampleId       ; readSampleId        = true;
				} else if(0 == token.compare("SCANID:"        )) {iss >> scanId         ; readScanId          = true;
				} else if(0 == token.compare("Phase"          )) {
					//check that all attributes for previous phase were read
					std::stringstream ss;
					ss << phaseList.size();
					if(    !readPhaseMaterial ) throw std::runtime_error("ang missing material name for phase "     + ss.str());
					if(    !readPhaseFormula  ) throw std::runtime_error("ang missing formula for phase "           + ss.str());
					if(    !readPhaseInfo     ) throw std::runtime_error("ang missing info for phase "              + ss.str());
					if(    !readPhaseSymmetry ) throw std::runtime_error("ang missing symmetry for phase "          + ss.str());
					if(    !readPhaseLattice  ) throw std::runtime_error("ang missing lattice constants for phase " + ss.str());
					if(    !readPhaseHkl      ) throw std::runtime_error("ang missing hkl families for phase "      + ss.str());
					if(6 != phaseElasticCount ) throw std::runtime_error("ang missing elastic constants for phase " + ss.str());
					if(    !readPhaseCategories) throw std::runtime_error("ang missing categories for phase "       + ss.str());
					if(    !phaseList.empty()  ) {
						if(targetFamilies < phaseList.back().hklFam.size())
							throw std::runtime_error("ang missing some hkl families for phase " + ss.str());
					}
					targetFamilies = phaseElasticCount = 0;
					readPhaseSymmetry = readPhaseMaterial = readPhaseFormula = readPhaseInfo = false;
					readPhaseLattice = readPhaseHkl = readPhaseCategories = false;

					//add a new blank phase to the list
					phaseList.resize(phaseList.size() + 1);
					iss >> phaseList.back().num;
				} else if(0 == token.compare("MaterialName"  )) {iss >> phaseList.back().name; readPhaseMaterial = true;
				} else if(0 == token.compare("Formula"       )) {iss >> phaseList.back().form; readPhaseFormula  = true;
				} else if(0 == token.compare("Info"          )) {iss >> phaseList.back().info; readPhaseInfo     = true;
				} else if(0 == token.compare("Symmetry"      )) {iss >> phaseList.back().sym ; readPhaseSymmetry = true;
				} else if(0 == token.compare("NumberFamilies")) {
					iss >> targetFamilies;//read the number of families
					phaseList.back().hklFam.reserve(targetFamilies);//allocate space for the families
					readPhaseHkl = true;
				} else if(0 == token.compare("LatticeConstants")) {
					iss >> phaseList.back().lat[0]
					    >> phaseList.back().lat[1]
					    >> phaseList.back().lat[2]
					    >> phaseList.back().lat[3]
					    >> phaseList.back().lat[4]
					    >> phaseList.back().lat[5];
					readPhaseLattice = true;
				} else if(0 == token.compare("hklFamilies")) {
					phaseList.back().hklFam.resize(phaseList.back().hklFam.size() + 1);//add new family (space was already reserved)
					iss >> phaseList.back().hklFam.back().hkl[0]
					    >> phaseList.back().hklFam.back().hkl[1]
					    >> phaseList.back().hklFam.back().hkl[2]
					    >> phaseList.back().hklFam.back().useIdx
					    >> phaseList.back().hklFam.back().intensity
					    >> phaseList.back().hklFam.back().showBands;
				} else if(0 == token.compare("ElasticConstants")) {
					iss >> phaseList.back().el[6*phaseElasticCount + 0]
					    >> phaseList.back().el[6*phaseElasticCount + 1]
					    >> phaseList.back().el[6*phaseElasticCount + 2]
					    >> phaseList.back().el[6*phaseElasticCount + 3]
					    >> phaseList.back().el[6*phaseElasticCount + 4]
					    >> phaseList.back().el[6*phaseElasticCount + 5];
					++phaseElasticCount;
				} else if(0 == token.compare(0, 10, "Categories")) {//tsl doesn't print space between categories and first number
					//rewind to first character after categories key
					iss.seekg((size_t) iss.tellg() + 10 - token.length());
					std::copy(std::istream_iterator<size_t>(iss), std::istream_iterator<size_t>(), std::back_inserter(phaseList.back().cats));
					readPhaseCategories = true;
				} else {throw std::runtime_error("unknown ang header keyword '" + token + "'");}
			}
		}

		//check that all values of final phase were read
		std::stringstream ss;
		ss << phaseList.size();
		if(    !readPhaseMaterial  ) throw std::runtime_error("ang missing material name for phase "     + ss.str());
		if(    !readPhaseFormula   ) throw std::runtime_error("ang missing formula for phase "           + ss.str());
		if(    !readPhaseInfo      ) throw std::runtime_error("ang missing info for phase "              + ss.str());
		if(    !readPhaseSymmetry  ) throw std::runtime_error("ang missing symmetry for phase "          + ss.str());
		if(    !readPhaseLattice   ) throw std::runtime_error("ang missing lattice constants for phase " + ss.str());
		if(    !readPhaseHkl       ) throw std::runtime_error("ang missing hkl families for phase "      + ss.str());
		// if(6 != phaseElasticCount  ) throw std::runtime_error("ang missing elastic constants for phase " + ss.str());
		// if(    !readPhaseCategories) throw std::runtime_error("ang missing categories for phase "        + ss.str());
		if(    !phaseList.empty()  ) {
			if(targetFamilies < phaseList.back().hklFam.size())
				throw std::runtime_error("ang missing some hkl families for phase " + ss.str());
		}

		//make sure all header values were read
		if(!readPixPerUm       ) throw std::runtime_error("missing ang header value TEM_PIXperUM"   );
		if(!readXStar          ) throw std::runtime_error("missing ang header value x-star"         );
		if(!readYStar          ) throw std::runtime_error("missing ang header value y-star"         );
		if(!readZStar          ) throw std::runtime_error("missing ang header value z-star"         );
		if(!readWorkingDistance) throw std::runtime_error("missing ang header value WorkingDistance");
		if(!readGridType       ) throw std::runtime_error("missing ang header value GRID"           );
		if(!readXStep          ) throw std::runtime_error("missing ang header value XSTEP"          );
		if(!readYStep          ) throw std::runtime_error("missing ang header value YSTEP"          );
		if(!readColsOdd        ) throw std::runtime_error("missing ang header value NCOLS_ODD"      );
		if(!readColsEven       ) throw std::runtime_error("missing ang header value NCOLS_EVEN"     );
		if(!readRows           ) throw std::runtime_error("missing ang header value NROWS"          );
		if(!readOperatorName   ) throw std::runtime_error("missing ang header value OPERATOR"       );
		if(!readSampleId       ) throw std::runtime_error("missing ang header value SAMPLEID"       );
		if(!readScanId         ) throw std::runtime_error("missing ang header value SCANID"         );

		//extract first line of data without advancing stream
		const std::streamoff start = is.tellg();//save position of data start
		is.getline(line, sizeof(line));//copy first data line
		is.seekg(start);//rewind stream to data start

		//get number of tokens
		size_t tokenCount = 0;
		std::istringstream iss(line);
		while(iss >> token) tokenCount++;//count number of values in first data line
		if(tokenCount < 8) {//make sure there are enough columns
			std::stringstream ss;
			ss << "unexpected number of ang values per point (got " << tokenCount << ", expected at least 8)";
			throw std::runtime_error(ss.str());
		}
		return tokenCount;
	}

	//@brief: read ang data using an input stream
	//@param is: input stream set data start
	//@param tokens: number of tokens per point
	//@return: number of points (rows) parsed
	size_t OrientationMap::readAngData(std::istream& is, size_t tokens) {
		char line[512];//most ang files have 128 byte lines including '\n' so this should be plenty
		bool evenRow = true;
		size_t pointsRead = 0;
		size_t completeRowPoints = 0;
		size_t currentCol = nColsEven - 1;
		const size_t totalPoints = iq.size();
		const bool readSem = tokens > 8;
		const bool readFit = tokens > 9;
		while(pointsRead < totalPoints) {//keep going until we run out of point
			char* data = line;//get pointer to line start
			if(!is.getline(line, sizeof(line))) break;//get next line
			const size_t i = completeRowPoints + currentCol;//get index of point currently being parsed
			eu   [3*i  ] =      strtof (data, &data    );//parse first euler angle
			eu   [3*i+1] =      strtof (data, &data    );//parse second euler angle
			eu   [3*i+2] =      strtof (data, &data    );//parse third euler angle
			x    [i]     =      strtof (data, &data    );//parse x
			y    [i]     =      strtof (data, &data    );//parse y
			iq   [i]     =      strtof (data, &data    );//parse image quality
			ci   [i]     =      strtof (data, &data    );//parse confidence index
			phase[i]     = std::strtoul(data, &data, 10);//parse phase (as base 10)
			if(readSem) {//are there 9 or more tokens?
				sem[i]     = strtof(data, &data);//parse SE signal
				if(readFit) {//are there 10 or more tokens?
					fit[i] = strtof(data, NULL);//parse fit
				}
			}
			pointsRead++;//increment number of points parsed
			if(0 == currentCol--) {//decrement current column and check if we've reached a new row
				completeRowPoints += evenRow ? nColsEven : nColsOdd;//update increment to start of current row
				evenRow = !evenRow;//are we currently on an even or odd row?
				currentCol = evenRow ? nColsEven - 1 : nColsOdd - 1;//get number of point in new row
			}
		}
		return pointsRead;
	}

	size_t OrientationMap::readAngDataMemMap(std::string fileName, std::streamoff offset, size_t tokens) {
		//open memory mapped file
		const memorymap::File mapped(fileName, memorymap::Hint::Sequential);
		char* data = const_cast<char*>(mapped.constData()) + offset;// !!! this pointer is read only, writing to it will cause undefined behavior (const_cast is for strtof etc)
		uint64_t fileBytes = mapped.size();//it is our responsibility to not go past the end of the memory map

		//parse data
		bool evenRow = true;
		size_t pointsRead = 0;
		size_t completeRowPoints = 0;
		size_t currentCol = nColsEven - 1;
		size_t totalPoints = iq.size();
		uint64_t bytesRead = (uint64_t)offset+1;//skipped header bytes count towards file size
		const bool readSem  = tokens >  8;
		const bool readFit  = tokens >  9;
		const bool extraTok = tokens > 10;
		while(pointsRead < totalPoints && bytesRead < fileBytes) {//keep going until we run out of points or file
			char* dStart = data;//save pointer to line start
			const size_t i = completeRowPoints + currentCol;//get index of point currently being parsed
			eu   [3*i  ] =      strtof (data, &data    );//parse first euler angle
			eu   [3*i+1] =      strtof (data, &data    );//parse second euler angle
			eu   [3*i+2] =      strtof (data, &data    );//parse third euler angle
			x    [i]     =      strtof (data, &data    );//parse x
			y    [i]     =      strtof (data, &data    );//parse y
			iq   [i]     =      strtof (data, &data    );//parse image quality
			ci   [i]     =      strtof (data, &data    );//parse confidence index
			phase[i]     = std::strtoul(data, &data, 10);//parse phase (as base 10)
			if(readSem) {//are there 9 or more tokens?
				sem[i] = strtof(data, &data);//parse SE signal
				if(readFit) {//are there 10 or more tokens?
					fit[i] = strtof(data, &data);//parse fit
					if(extraTok) while('\n' != *data) ++data;//skip extra tokens until the end of the line
				}
			}
			bytesRead += data - dStart;//count number of bytes read (make sure we don't read past the end of the file)
			pointsRead++;//increment number of points parsed
			if(0 == currentCol--) {//decrement current column and check if we've reached a new row
				completeRowPoints += evenRow ? nColsEven : nColsOdd;//update increment to start of current row
				evenRow = !evenRow;//are we currently on an even or odd row?
				currentCol = evenRow ? nColsEven - 1 : nColsOdd - 1;//get number of point in new row
			}
		}
		return pointsRead;
	}
}

#endif//_tsl_h_