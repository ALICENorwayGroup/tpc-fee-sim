#include <vector>
#include <string>

struct Point
{
	float x;
	float y;
};
typedef std::vector<std::string> StringVector;
typedef	StringVector MultiPoint;
/*
	Class which contains a graph.
	Used to help us generate a graphPlot.
*/



class Graph
{

	public:

		Graph();
		Graph(std::vector< Point > points, std::string _xLabel, std::string _yLabel, std::string _name);
		Graph(std::vector< MultiPoint > points, StringVector labels, std::string _name);

		bool writeGraphToFile(bool append);
		bool writeMultiGraphToFile(bool);

		inline void addGraphPoint(Point p){ graphPoints.push_back(p); };
		inline std::vector<Point> getGraph(){ return graphPoints; };
	
		inline std::string getName(){ return name; };
		inline void setNAme(std::string n) { name = n; };		

		inline std::string getXLabel(){ return xLabel; };
		inline void setXLabel(std::string label) { xLabel = label; };
		inline std::string getYLabel(){ return yLabel; };
		inline void setYLabel(std::string label) { yLabel = label; };

		inline std::string getLabel(int pos){return labels[pos]; };

	private:
		std::vector< Point > graphPoints;
		std::vector< MultiPoint> graphMultiPoints;
		StringVector labels;
		std::string xLabel;
		std::string yLabel;	
		std::string name;
};