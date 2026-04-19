#include "node.h"

ProcessNode::ProcessNode(const qefirParams qefParams) : buffer{0},
                                                        qefParams(qefParams),
                                                        _phase{0},
                                                        _freqHz{432}
{

}

ProcessNode::~ProcessNode()
{

}
