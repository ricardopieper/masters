/* ***************************************************************************
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  As a special exception, you may use this file as part of a free software
 *  library without restriction.  Specifically, if other files instantiate
 *  templates or use macros or inline functions from this file, or you compile
 *  this file and link it with other files to produce an executable, this
 *  file does not by itself cause the resulting executable to be covered by
 *  the GNU General Public License.  This exception does not however
 *  invalidate any other reasons why the executable file might be covered by
 *  the GNU General Public License.
 *
 ****************************************************************************
 *  Authors: Dalvan Griebler <dalvangriebler@gmail.com>
 *         
 *  Copyright: GNU General Public License
 *  Description: This program generates two files (inputA.txt and inputB.txt) that will
 *  be the input stream to fill up the A and B matrices to apply dot product over fixed 
 * 	size and write the results in an output file (output.txt)
 *  Version: 2.0 (02/09/2018)
 *  Compilation Command: g++ -std=c++1y dot_product_stream.cpp -o exe
 ****************************************************************************
*/

#include <iostream>
#include <fstream>
#include <cmath>
#include "tbb/tbb.h"
#define MX 30


void val(long int **A, long int **B, long int **C, long int valA, long int valB){
	for(long int i=0; i<MX; i++){
		for(long int j=0; j<MX; j++){
			A[i][j] = valA;
			B[i][j] = valB;
			C[i][j] = 0;
		}
	}	
}
std::ofstream stream_out;
std::ifstream stream_inA;
std::ifstream stream_inB;


struct Work {
	long int valA;
	long int valB;
};

struct Matrices {
	long int** A;
	long int** B;
	long int** C;
};

class GenerateWork : public tbb::filter {
	private:
	 	std::ifstream stream_inA;
	 	std::ifstream stream_inb;
	
	public:
	GenerateWork():tbb::filter(tbb::filter::serial){
		stream_inA.open("inputA.txt",std::ios::in);
		stream_inB.open("inputB.txt",std::ios::in);
	}
	void* operator()(void* in_task) {
        if(!stream_inB.eof()){
			long int valA;
			long int valB;
			stream_inA >> valA;
			stream_inB >> valB;
		
			long int **A = new long int*[MX];
			long int **B = new long int*[MX];
			long int **C = new long int*[MX];
			
			for (long int i=0; i < MX; i++){
				A[i] = new long int[MX];
				B[i] = new long int[MX];
				C[i] = new long int[MX];
			}
			
			val(A,B,C,valA,valB);

			for(long int i=0; i<MX; i++){
				for(long int j=0; j<MX; j++){
					A[i][j] = valA;
					B[i][j] = valB;
					C[i][j] = 0;
				}
			}
			return new Matrices { A = A, B = B, C = C };
		} else {
			return NULL;
		}
	}
	~GenerateWork(){
		stream_inA.close();
		stream_inB.close();
	}
};

class MatrixMultiplier : public tbb::filter {
	
	public:
	MatrixMultiplier():tbb::filter(tbb::filter::parallel){}
	void* operator()(void* in_task) {
		Matrices* m = static_cast<Matrices*>(in_task); 

		long int **A = m->A;
	  	long int **B = m->B;
	  	long int **C = m->C;	  

		for(long int i=0; i<MX; i++){
			for(long int j=0; j<MX; j++){
				for(long int k=0; k<MX; k++){
					C[i][j] += (A[i][k] * B[k][j]);
				}
			}
		}

		delete[] m->A;
		delete[] m->B;

		return m;
	}
};

class Output : public tbb::filter {

	private:
	std::ofstream stream_out;
	public:
	Output():tbb::filter(tbb::filter::serial_in_order){
		stream_out.open("output.txt",std::ios::out);
	}
	void* operator()(void* in_task) {
		Matrices* m = static_cast<Matrices*>(in_task); 

	  	long int **C = m->C;	  

		for(long int i=0; i<MX; i++){
			for(long int j=0; j<MX; j++){
				stream_out << C[i][j];
				stream_out << " ";

			}
			stream_out << "\n";
		}

		stream_out << "----------------------------------\n";
		
		delete[] m->C;
		delete m;
		return NULL;
	}
	~Output() {
		stream_out.close();
	}
};

void dp_pipelined() {
	tbb::task_scheduler_init(1);
	tbb::pipeline pipe;
	GenerateWork gw;
	MatrixMultiplier mm;
	Output o;
	pipe.add_filter(gw);
	pipe.add_filter(mm);
	pipe.add_filter(o);
	pipe.run(1);
}

int main(int argc, char const *argv[]){
	dp_pipelined();
	return 0;
}