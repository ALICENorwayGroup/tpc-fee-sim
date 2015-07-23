#include "Graph.h"
#include <fstream>


Graph::Graph(){

}

Graph::Graph(std::vector<Point> points, std::string _xLabel, std::string _yLabel, std::string _name){
	graphPoints = points;
	xLabel = _xLabel;
	yLabel = _yLabel;
	name = _name;
}

Graph::Graph(std::vector< MultiPoint > points, std::vector<std::string> _labels, std::string _name){
    graphMultiPoints = points;
    labels = _labels;
    name = _name;
}

bool Graph::writeMultiGraphToFile(bool append){
    std::ofstream GraphFile;
    if(append){
        GraphFile.open("graph-"+name+".csv", std::ofstream::out|std::ofstream::ate|std::ofstream::app);
				//GraphFile << '\n';
    } else {
        GraphFile.open("graph-"+name+".csv");
				GraphFile << '\n';
				for (std::vector<std::string>::iterator it = labels.begin(); it != labels.end(); ++it)
		    {
		        GraphFile << *it << ";";
		    }
				GraphFile << '\n';
    }
		GraphFile << '\n';
    for (std::vector< MultiPoint>::iterator it = graphMultiPoints.begin(); it != graphMultiPoints.end(); it++)
    {
        for (std::vector<std::string>::iterator jt = it->begin(); jt != it->end(); jt++)
        {
            GraphFile << *jt << ";";
        }
        GraphFile << '\n';
    }
     GraphFile.close();
}

bool Graph::writeGraphToFile(bool append){
     std::ofstream GraphFile;
    if(append){
        GraphFile.open("graph-"+name+".csv", std::ofstream::app);
    } else {
        GraphFile.open("graph-"+name+".csv");
    }
	         //Opening file to print info to
    GraphFile << xLabel <<";" << yLabel << std::endl;          //Headings for file
    for (int i = 0; i < graphPoints.size(); i++)
    {
    	GraphFile << graphPoints[i].x << ";" << graphPoints[i].y << std::endl;
    }

     GraphFile.close();
}
