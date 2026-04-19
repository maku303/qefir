#include "filterNode.h"

FilterNode::FilterNode(const qefirParams& qefParams): buffer{0}, qefParams{qefParams}, _phase{0}, _freqHz{0}
{
}

FilterNode::~FilterNode()
{
}