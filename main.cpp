#include <chrono>

struct  Timer {
	std::chrono::high_resolution_clock::time_point tp;
	Timer() : tp(std::chrono::high_resolution_clock::now()) {}

	//@brief: compute time since previous call (or construction)
	//@return: time in seconds
	double poll() {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();//get current time
		std::chrono::duration<double> elapsed = now - tp;//compute duration since last poll
		tp = now;//update last polled time
		return elapsed.count();//return elapsed time in fractional seconds
	}
};

#include <iostream>
#include "tsl.hpp"

int main() {
	//read the ang file and time how long it takes
	Timer timer;
	std::string fileName = "filename.ang";
	tsl::OrientationMap om(fileName);
	std::cout << "read '" << fileName << "' in " << timer.poll() << "s\n";

	//print out some header information
	std::cout << "pattern center  : " << om.xStar << ' ' << om.yStar << ' ' << om.zStar << '\n';
	std::cout << "working distance: " << om.workingDistance << '\n';
	std::cout << "pixel size      : " << om.xStep << " x " << om.yStep << '\n';
	std::cout << "scan size       : ("<< om.nColsOdd << '/' << om.nColsEven << ") x " << om.nRows << '\n';
	std::cout << "grid type       : " << om.gridType << '\n';
	std::cout << "operator        : " << om.operatorName << '\n';
	std::cout << "sample ID       : " << om.sampleId << '\n';
	std::cout << "scan ID         : " << om.scanId << '\n';

	//print out the phases
	std::cout << om.phaseList.size() << " phase(s):\n";
	for(const tsl::Phase& phase : om.phaseList) {
		std::cout << "\t" << phase.num << ": " << phase.name << '\n';
		std::cout << "\t\tformula : " << phase.form << '\n';
		std::cout << "\t\tinfo    : " << phase.info << '\n';
		std::cout << "\t\tsymmetry: " << phase.sym  << '\n';
		std::cout << "\t\t" << phase.hklFam.size() << ":\n";
		std::cout << "\t\t\thkl\t\tuse\tintensity\tshow\n";
		const size_t sampleHkl = 3;
		for(size_t i = 0; i < std::min<size_t>(sampleHkl, phase.hklFam.size()); i++) {
			const tsl::HKLFamily& f = phase.hklFam[i];
			std::cout << "\t\t\t" << f.hkl[0] << ' ' << f.hkl[1] << ' ' << f.hkl[2] << '\t' << f.useIdx << '\t' << f.intensity << "\t\t\t" << f.showBands << '\n';
		}
		if(phase.hklFam.size() > sampleHkl) {
			std::cout << "\t\t\t...\n";
		}
	}

	//now print out a snippet of the actual data
	std::cout << "\neuler0\teuler1\teuler2\tX\tY\tIQ\tCI\tPhase\n";
	const size_t samplePoints = 5;
	if(2 * samplePoints >= om.iq.size()) {
		//print all the points
		for(size_t i = 0; i < om.iq.size(); i++) {
			std::cout << om.eu[3*i] << '\t' << om.eu[3*i+1] << '\t' << om.eu[3*i+2] << '\t' << om.x[i] << '\t' << om.y[i] << '\t' << om.iq[i] << '\t' << om.ci[i] << '\t' << om.phase[i] << '\n';
		}
	} else {
		//print the start and end of the data
		for(size_t i = 0; i < samplePoints; i++) {
			std::cout << om.eu[3*i] << '\t' << om.eu[3*i+1] << '\t' << om.eu[3*i+2] << '\t' << om.x[i] << '\t' << om.y[i] << '\t' << om.iq[i] << '\t' << om.ci[i] << '\t' << om.phase[i] << '\n';
		}
		std::cout << "...\n...\n...\n";
		for(size_t i = om.iq.size() - samplePoints; i < om.iq.size(); i++) {
			std::cout << om.eu[3*i] << '\t' << om.eu[3*i+1] << '\t' << om.eu[3*i+2] << '\t' << om.x[i] << '\t' << om.y[i] << '\t' << om.iq[i] << '\t' << om.ci[i] << '\t' << om.phase[i] << '\n';
		}
	}

	return 0;
}