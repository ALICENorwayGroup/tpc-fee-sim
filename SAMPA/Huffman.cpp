#include "Huffman.h"

Huffman::Huffman(){

}

INode* Huffman::BuildTree(const int (&frequencies)[constants::HUFFMAN_RANGE])
{
  std::priority_queue<INode*, std::vector<INode*>, NodeCmp> trees;

  for (int i = 0; i < constants::HUFFMAN_RANGE; ++i)
  {
    if(frequencies[i] != 0)
    trees.push(new LeafNode(frequencies[i], (uint16_t)i));
  }
  while (trees.size() > 1)
  {
    INode* childR = trees.top();
    trees.pop();

    INode* childL = trees.top();
    trees.pop();

    INode* parent = new InternalNode(childR, childL);
    trees.push(parent);
  }
  return trees.top();
}

void Huffman::GenerateCodes(const INode* node, const HuffCode& prefix, HuffCodeMap& outCodes)
{
  if (const LeafNode* lf = dynamic_cast<const LeafNode*>(node))
  {
    outCodes[lf->c] = prefix;
  }
  else if (const InternalNode* in = dynamic_cast<const InternalNode*>(node))
  {
    HuffCode leftPrefix = prefix;
    leftPrefix.push_back(false);
    GenerateCodes(in->left, leftPrefix, outCodes);

    HuffCode rightPrefix = prefix;
    rightPrefix.push_back(true);
    GenerateCodes(in->right, rightPrefix, outCodes);
  }
}

void Huffman::WriteCodesToFile(const char *filename, const HuffCodeMap& codes){
  std::ofstream os(filename);

  for(HuffCodeMap::const_iterator it = codes.begin(); it != codes.end(); it++){
    os << it->first << " ";
    std::copy(it->second.begin(), it->second.end(),
    std::ostream_iterator<bool>(os));
    os << std::endl;

  }
}

void Huffman::CodesFromFile(const char *filename, HuffCodeMap& outCodes){
  std::ifstream is(filename);
  std::string line;
  if (!is.good()) {
    std::cerr << "can not open file " << filename << " for reading of Huffman code table" << std::endl;
    return;
  }

  while(!is.eof()){
    std::vector<bool> codes;
    std::getline(is, line);
    try{
      uint16_t val = std::stoi(line.substr(0, line.find(" ")));
      std::string code = line.substr(line.find(" ")+1);
      for(char& c : code){
        codes.push_back(c == '1');
        outCodes[val] = codes;
      }
    } catch(const std::invalid_argument& ia){

    }




  }

}

void Huffman::CreateTree(const std::vector<uint16_t>& words, HuffCodeMap& outCodes){
  int frequencies[constants::HUFFMAN_RANGE] = {0};

  for(int i = 0; i < words.size(); i++){
    uint16_t ptr = words.at(i);
    if(ptr < constants::HUFFMAN_RANGE)
    ++frequencies[ptr];
  }

  INode* root = BuildTree(frequencies);

  GenerateCodes(root, HuffCode(), outCodes);
  delete root;
}
