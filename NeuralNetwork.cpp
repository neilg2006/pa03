// includes
#include "NeuralNetwork.hpp"
using namespace std;



// NeuralNetwork -----------------------------------------------------------------------------------------------------------------------------------

// STUDENT TODO: IMPLEMENT
void NeuralNetwork::eval() {
    evaluating = true;
}

// STUDENT TODO: IMPLEMENT
void NeuralNetwork::train() {
    evaluating = false;
    batchSize  = 0;
}

// STUDENT TODO: IMPLEMENT
void NeuralNetwork::setLearningRate(double lr) {
    learningRate = lr;
}

// STUDENT TODO: IMPLEMENT
void NeuralNetwork::setInputNodeIds(std::vector<int> inputNodeIds) {
    this->inputNodeIds = inputNodeIds;
}

// STUDENT TODO: IMPLEMENT
void NeuralNetwork::setOutputNodeIds(std::vector<int> outputNodeIds) {
    this->outputNodeIds = outputNodeIds;
}


vector<int> NeuralNetwork::getInputNodeIds() const {
    return inputNodeIds; 
}


vector<int> NeuralNetwork::getOutputNodeIds() const {
    return outputNodeIds; 
}


vector<double> NeuralNetwork::predict(DataInstance instance) {
    flush(); 

    

    vector<double> input = instance.x;

    
    if (input.size() != inputNodeIds.size()) {
        cerr << "input size mismatch." << endl;
        cerr << "\tNeuralNet expected input size: " << inputNodeIds.size() << endl;
        cerr << "\tBut got: " << input.size() << endl;
        return vector<double>();
    }

    vector<bool> enqueued(nodes.size(), false);
    queue<int> q;

    
    for (int i = 0; i < inputNodeIds.size(); ++i) {
        int inId = inputNodeIds.at(i);
        NodeInfo* inNode = nodes.at(inId);
        inNode->preActivationValue = input.at(i); 
        enqueued[inId] = true;
        q.push(inId);
    }

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

       
  
        visitPredictNode(curr);

    

        for (auto& kv : adjacencyList.at(curr)) {
            const Connection& c = kv.second;
            visitPredictNeighbor(c);  

            int dest = c.dest;
            if (!enqueued[dest]) {
                enqueued[dest] = true;
                q.push(dest);
            }
        }
    }

    

    vector<double> output;
    for (int i = 0; i < outputNodeIds.size(); i++) {
        int dest = outputNodeIds.at(i);
        NodeInfo* outputNode = nodes.at(dest);
        output.push_back(outputNode->postActivationValue);
    }

    if (evaluating) {
        flush();
    } else {
        batchSize++;
        contribute(instance.y, output.at(0));   
    }
    return output;
}
// STUDENT TODO: IMPLEMENT
bool NeuralNetwork::contribute(double y, double p) {
    contributions.clear();

    double incomingContribution = 0;
    double outgoingContribution = 0;
    NodeInfo* currNode = nullptr;

    for(int i = 0; i < inputNodeIds.size(); i++){
        for(auto& n: adjacencyList[inputNodeIds[i]]){
            
            if(contributions.count(n.first)){
                incomingContribution = contributions[n.first];
            }

            else{
                incomingContribution = contribute(n.first, y, p);
                contributions [n.first] = incomingContribution;
            }
            visitContributeNeighbor(n.second, incomingContribution, outgoingContribution);
        }
    }
    
    flush();

    return true;
}
// STUDENT TODO: IMPLEMENT
double NeuralNetwork::contribute(int nodeId, const double& y, const double& p) {

    double incomingContribution = 0;
    double outgoingContribution = 0;
    NodeInfo* currNode = nodes.at(nodeId);

    if (contributions.count(nodeId)) {
        return contributions[nodeId];
    }
    else{
        for (auto& nds: adjacencyList[nodeId]){
            if (contributions.count(nds.first)){
                incomingContribution = contributions[nds.first];
            }
            else{
                incomingContribution = contribute(nds.first, y, p);
                contributions[nds.first] = incomingContribution;
            }
            visitContributeNeighbor (nds.second, incomingContribution, outgoingContribution);
        }
    }

    if (adjacencyList.at(nodeId).empty()) {
        outgoingContribution = -1 * ((y - p) / (p * (1 - p)));
    } 

    visitContributeNode(nodeId, outgoingContribution);
    
    return outgoingContribution;
}
// STUDENT TODO: IMPLEMENT
bool NeuralNetwork::update() {
  
    for (auto &nodePtr : nodes) {
        nodePtr->bias  -= learningRate * nodePtr->delta;
        nodePtr->delta  = 0.0;
    }

    for (int u = 0; u < adjacencyList.size(); ++u) {
        for (auto &kv : adjacencyList[u]) {
            Connection &c = kv.second;
            c.weight -= learningRate * c.delta;
            c.delta   = 0.0;
        }
    }


    flush();
    return true;
    
}




// Feel free to explore the remaining code, but no need to implement past this point

// ----------- YOU DO NOT NEED TO TOUCH THE REMAINING CODE -----------------------------------------------------------------
// ----------- YOU DO NOT NEED TO TOUCH THE REMAINING CODE -----------------------------------------------------------------
// ----------- YOU DO NOT NEED TO TOUCH THE REMAINING CODE -----------------------------------------------------------------
// ----------- YOU DO NOT NEED TO TOUCH THE REMAINING CODE -----------------------------------------------------------------
// ----------- YOU DO NOT NEED TO TOUCH THE REMAINING CODE -----------------------------------------------------------------







// Constructors
NeuralNetwork::NeuralNetwork() : Graph(0) {
    learningRate = 0.1;
    evaluating = false;
    batchSize = 0;
}

NeuralNetwork::NeuralNetwork(int size) : Graph(size) {
    learningRate = 0.1;
    evaluating = false;
    batchSize = 0;
}

NeuralNetwork::NeuralNetwork(string filename) : Graph() {
    // open file
    ifstream fin(filename);

    // error check
    if (fin.fail()) {
        cerr << "Could not open " << filename << " for reading. " << endl;
        exit(1);
    }

    // load network
    loadNetwork(fin);
    learningRate = 0.1;
    evaluating = false;
    batchSize = 0;

    // close file
    fin.close();
}

NeuralNetwork::NeuralNetwork(istream& in) : Graph() {
    loadNetwork(in);
    learningRate = 0.1;
    evaluating = false;
    batchSize = 0;
}

void NeuralNetwork::loadNetwork(istream& in) {
    int numLayers(0), totalNodes(0), numNodes(0), weightModifications(0), biasModifications(0); string activationMethod = "identity";
    string junk;
    in >> numLayers; in >> totalNodes; getline(in, junk);
    if (numLayers <= 1) {
        cerr << "Neural Network must have at least 2 layers, but got " << numLayers << " layers" << endl;
        exit(1);
    }

    // resize network to accomodate expected nodes.
    resize(totalNodes);
    this->size = totalNodes;

    int currentNodeId(0);

    vector<int> previousLayer;
    vector<int> currentLayer;
    for (int i = 0; i < numLayers; i++) {
        currentLayer.clear();
        //  For each layer

        // get nodes for this layer and activation method
        in >> numNodes; in >> activationMethod; getline(in, junk);

        for (int j = 0; j < numNodes; j++) {
            // For every node, add a new node to the network with proper activationMethod
            // initialize bias to 0.
            updateNode(currentNodeId, NodeInfo(activationMethod, 0, 0));
            // This node has an id of currentNodeId
            currentLayer.push_back(currentNodeId++);
        }

        if (i != 0) {
            // There exists a previous layer, now we set out connections
            for (int k = 0; k < previousLayer.size(); k++) {
                for (int w = 0; w < currentLayer.size(); w++) {

                    // Initialize an initial weight of a sample from the standard normal distribution
                    updateConnection(previousLayer.at(k), currentLayer.at(w), sample());
                }
            }
        }

        // Crawl forward.
        previousLayer = currentLayer;
        layers.push_back(currentLayer);
    }
    in >> weightModifications; getline(in, junk);
    int v(0),u(0); double w(0), b(0);

    // load weights by updating connections
    for (int i = 0; i < weightModifications; i++) {
        in >> v; in >> u; in >> w; getline(in , junk);
        updateConnection(v, u, w);
    }

    in >> biasModifications; getline(in , junk);

    // load biases by updating node info
    for (int i = 0; i < biasModifications; i++) {
        in >> v; in >> b; getline(in, junk);
        NodeInfo* thisNode = getNode(v);
        thisNode->bias = b;
    }

    setInputNodeIds(layers.at(0));
    setOutputNodeIds(layers.at(layers.size()-1));
}

void NeuralNetwork::visitPredictNode(int vId) {
    // accumulate bias, and activate
    NodeInfo* v = nodes.at(vId);
    v->preActivationValue += v->bias;
    v->activate();
}

void NeuralNetwork::visitPredictNeighbor(Connection c) {
    NodeInfo* v = nodes.at(c.source);
    NodeInfo* u = nodes.at(c.dest);
    double w = c.weight;
    u->preActivationValue += v->postActivationValue * w;
}

void NeuralNetwork::visitContributeNode(int vId, double& outgoingContribution) {
    NodeInfo* v = nodes.at(vId);
    outgoingContribution *= v->derive();
    
    //contribute bias derivative
    v->delta += outgoingContribution;
}

void NeuralNetwork::visitContributeNeighbor(Connection& c, double& incomingContribution, double& outgoingContribution) {
    NodeInfo* v = nodes.at(c.source);
    // update outgoingContribution
    outgoingContribution += c.weight * incomingContribution;

    // accumulate weight derivative
    c.delta += incomingContribution * v->postActivationValue;
}

void NeuralNetwork::flush() {
    // set every node value to 0 to refresh computation.
    for (int i = 0; i < nodes.size(); i++) {
        nodes.at(i)->postActivationValue = 0;
        nodes.at(i)->preActivationValue = 0;
    }
    contributions.clear();
    batchSize = 0;
}

double NeuralNetwork::assess(string filename) {
    DataLoader dl(filename);
    return assess(dl);
}

double NeuralNetwork::assess(DataLoader dl) {
    bool stateBefore = evaluating;
    evaluating = true;
    double count(0);
    double correct(0);
    vector<double> output;
    for (int i = 0; i < dl.getData().size(); i++) {
        DataInstance di = dl.getData().at(i);
        output = predict(di);
        if (static_cast<int>(round(output.at(0))) == di.y) {
            correct++;
        }
        count++;
    }

    if (dl.getData().empty()) {
        cerr << "Cannot assess accuracy on an empty dataset" << endl;
        exit(1);
    }
    evaluating = stateBefore;
    return correct / count;
}


void NeuralNetwork::saveModel(string filename) {
    ofstream fout(filename);
    
    fout << layers.size() << " " << getNodes().size() << endl;
    for (int i = 0; i < layers.size(); i++) {
        NodeInfo* layerNode = getNodes().at(layers.at(i).at(0));
        string activationType = getActivationIdentifier(layerNode->activationFunction);

        fout << layers.at(i).size() << " " << activationType << endl;
    }

    int numWeights = 0;
    int numBias = 0;
    stringstream weightStream;
    stringstream biasStream;
    for (int i = 0; i < nodes.size(); i++) {
        numBias++;
        biasStream << i << " " << nodes.at(i)->bias << endl;

        for (auto j = adjacencyList.at(i).begin(); j != adjacencyList.at(i).end(); j++) {
            numWeights++;
            weightStream << j->second.source << " " << j->second.dest << " " << j->second.weight << endl;
        }
    }

    fout << numWeights << endl;
    fout << weightStream.str();
    fout << numBias << endl;
    fout << biasStream.str();

    fout.close();


}

ostream& operator<<(ostream& out, const NeuralNetwork& nn) {
    for (int i = 0; i < nn.layers.size(); i++) {
        out << "layer " << i << ": ";
        for (int j = 0; j < nn.layers.at(i).size(); j++) {
            out << nn.layers.at(i).at(j) << " ";
        }
        out << endl;
    }
    // outputs the nn in dot format
    out << static_cast<const Graph&>(nn) << endl;
    return out;
}

