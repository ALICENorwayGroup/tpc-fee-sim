#include <iostream>
#include <queue>
#include <vector>
#include <map>
#include <climits> // for CHAR_BIT
#include <iterator>
#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "GlobalConstants.h"
#include <fstream>

typedef std::vector<bool> HuffCode;
typedef std::map<uint16_t, HuffCode> HuffCodeMap;
class INode
{
public:
    const int f;

    virtual ~INode() {}

protected:
    INode(int f) : f(f) {}
};

class InternalNode : public INode
{
public:
    INode *const left;
    INode *const right;

    InternalNode(INode* c0, INode* c1) : INode(c0->f + c1->f), left(c0), right(c1) {}
    ~InternalNode()
    {
        delete left;
        delete right;
    }
};

class LeafNode : public INode
{
public:
    const uint16_t c;

    LeafNode(int f, uint16_t c) : INode(f), c(c) {}
};

struct NodeCmp
{
    bool operator()(const INode* lhs, const INode* rhs) const { return lhs->f > rhs->f; }
};

class Huffman {
public:
  INode* BuildTree(const int (&frequencies)[constants::HUFFMAN_RANGE]);
  void GenerateCodes(const INode* node, const HuffCode& prefix, HuffCodeMap& outCodes);
  void WriteCodesToFile(const char *filename, const HuffCodeMap& codes);
  void CodesFromFile(const char *filename, HuffCodeMap& outCodes);
  void CreateTree(const std::vector<uint16_t>& words, HuffCodeMap& outCodes);

  Huffman();

};
