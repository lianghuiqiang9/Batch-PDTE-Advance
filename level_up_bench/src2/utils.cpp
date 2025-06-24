#include "utils.h"

#include <fstream>
#include <sys/random.h>
#include "json.hpp"

using namespace std;

Timer::Timer() {
    start_ = chrono::steady_clock::now();
    end_ = chrono::steady_clock::now();
}

void Timer::start() {
    start_ = chrono::steady_clock::now();
}

void Timer::end() {
    end_ = chrono::steady_clock::now();
}

long double Timer::end_and_get() {
    end_ = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_ - start_);
    return elapsed.count();
}

long double Timer::get_time_in_milliseconds() {
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_ - start_);
    return elapsed.count();
}

Node::Node(uint64_t threshold, uint64_t attr_idx, Node *left, Node *right): threshold{threshold}, attr_idx{attr_idx}, left{left}, right{right} {}

bool Node::is_leaf2() const {
    return this->left == nullptr && this->right == nullptr;
}


void print_node(Node& a, string tabs){
    if (a.is_leaf2()){
       cout << tabs << "(class: " << a.threshold << ")" << endl; 
    }else{
        cout << tabs
            << "(f: " << a.attr_idx 
            << ", t: " << a.threshold
            << ")" << endl; 
    }
}

void print(Node* a, string tabs) {
    if (a->is_leaf2()){
       print_node(*a, tabs);
    }else{
        print(a->right, tabs + "        ");
        print_node(*a, tabs);
        print(a->left, tabs + "        ");
    }
}

void print_tree(Node* a) {
    //std::shared_ptr<Node> tmp_a = std::make_shared<Node>(a);
    auto temp_a = a;
    print(temp_a, " ");
}


Node *build_tree_from_json(json::reference ref) {

    Node *ret = nullptr;

    if (!ref["leaf"].is_null()) {
        uint64_t threshold = ref["leaf"].get<uint64_t>();
        ret = new Node(threshold, 0, nullptr, nullptr);
    } else {

        uint64_t threshold = ref["internal"]["threshold"].get<uint64_t>();
        uint64_t attr_idx = ref["internal"]["feature"].get<uint64_t>();
        auto left = build_tree_from_json(ref["internal"]["left"]);
        auto right = build_tree_from_json(ref["internal"]["right"]);
        ret = new Node(threshold, attr_idx, left, right);

    }

    return ret;
}


Node * Node2(std::string filename) {
    std::ifstream file(filename);
    nlohmann::json ref = nlohmann::json::parse(file);
    auto ret = build_tree_from_json(ref);
    return ret;
}


void read_attr_vec(const string &fname, vector<uint64_t> &dest) {
    dest.clear();

    ifstream f(fname);
    string line;
    while(getline(f, line)) {
        istringstream idata_stream(line);
        int val;
        idata_stream >> val;
        dest.push_back(val);
    }
}

int extended_euclidean(int a, int b, int *x, int *y);

int mod_inverse(int a, int m) {
    int x = 0, y = 0;
    int g = extended_euclidean(a, m, &x, &y);
    assert(g == 1);
    return (x % m + m) % m;
}
 
// Function for extended Euclidean Algorithm
int extended_euclidean(int a, int b, int *x, int *y) {
    if(a == 0) {
        *x = 0;
        *y = 1;
        return b;
    }

    int x1 = 0, y1 = 0;
    int gcd = extended_euclidean(b % a, a, &x1, &y1);

    *x = y1 - (b / a) * x1;
    *y = x1;

    return gcd;
}

uint64_t rand_uint64() {
    char arr[8];
    auto some = getrandom(arr, 8, 0);
    uint64_t ret;
    for(int i = 0; i < 8; i++) {
        ret = (ret << 8) + arr[i];
    }
    return ret;
}
