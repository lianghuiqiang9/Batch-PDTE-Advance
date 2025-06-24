#include <iostream>
#include <fstream>
#include <cassert>
#include <iomanip>
#include "node.h"
#include "utils.h"
#include "cmp.h"
#include "pdte.h"
#include<seal/seal.h>
#include <stack>

using namespace std;
using namespace seal;

void pdte_rdcmp_rec(vector<Ciphertext>& out,Node& node, Evaluator *evaluator,GaloisKeys* gal_keys_server, RelinKeys *rlk_server, vector<vector<Ciphertext>> client_input, Plaintext one,int n, BatchEncoder *batch_encoder,int slot_count, int row_count, int num_cmps){
    if (node.is_leaf()){
        out.push_back(node.value);
    }else{
        //pearent     0           *  parent       1
        //left      0   1  right  *  left       1   0     right
        //note that the index of client_data in the tree starts from 0
        //cmp
        node.right->value = rdcmp(evaluator,rlk_server, n, node.threshold_bitv_plain, client_input[ node.feature_index ]);
        evaluator->negate(node.right->value, node.left->value);
        evaluator->add_plain_inplace(node.left->value, one);
        //travel
        evaluator->add_inplace(node.left->value, node.value);
        evaluator->add_inplace(node.right->value, node.value);
        pdte_rdcmp_rec(out, *(node.left), evaluator,gal_keys_server, rlk_server, client_input,one,n, batch_encoder,slot_count, row_count, num_cmps);
        pdte_rdcmp_rec(out, *(node.right), evaluator,gal_keys_server, rlk_server, client_input,one,n, batch_encoder,slot_count, row_count, num_cmps);
    }
}

void pdte_rdcmp_iter(
    vector<Ciphertext>& out,
    Node& root,
    Evaluator* evaluator,
    GaloisKeys* gal_keys_server,
    RelinKeys* rlk_server,
    const vector<vector<Ciphertext>>& client_input,
    const Plaintext& one,
    int n,
    BatchEncoder* batch_encoder,
    int slot_count,
    int row_count,
    int num_cmps
) {
    stack<StackFrame> stk;
    stk.push({ &root, false });

    while (!stk.empty()) {
        StackFrame frame = stk.top();
        stk.pop();

        Node* node = frame.node;

        if (node->is_leaf()) {
            out.push_back(node->value);
            continue;
        }

        if (!frame.visited) {
            node->right->value = rdcmp(
                evaluator,
                rlk_server,
                n,
                node->threshold_bitv_plain,
                client_input[node->feature_index]
            );

            evaluator->negate(node->right->value, node->left->value);
            evaluator->add_plain_inplace(node->left->value, one);
            evaluator->add_inplace(node->left->value, node->value);
            evaluator->add_inplace(node->right->value, node->value);

            // 标记第二次访问
            stk.push({ node, true });
            // 先右后左入栈，保证左子树先访问
            stk.push({ node->right.get(), false }); // 如果是智能指针，使用 get()
            stk.push({ node->left.get(), false });
        }
        // 第二次访问不需操作
    }
}


//g++ -o rdcmp_pdte -O3 rdcmp_pdte.cpp src/utils.cpp src/cmp.cpp src/node.cpp src/pdte.cpp -I./include -I /usr/local/include/SEAL-4.1 -lseal-4.1

int main(int argc, char* argv[]){

    string address_data;
    string address_tree;
    int n;
    int clr = 0;
    int data_m;
    int opt;
    int extra = 0;
    while ((opt = getopt(argc, argv, "ft:v:r:n:c:e:")) != -1) {
        switch (opt) {
        case 't':address_tree = string(optarg);break;
        case 'v':address_data = string(optarg);break;
        case 'r':data_m = atoi(optarg);break;
        case 'n':n = atoi(optarg);break;
        case 'c':clr = atoi(optarg);break;
        case 'e':extra = atoi(optarg);break;
        }
    }

    stringstream client_send;
    stringstream server_send;
    long client_send_commun=0;
    long server_send_commun=0;
    cout<<"******************************* step 1: client begin *******************************"<<endl;
    clock_t global_start = clock();
    clock_t start = clock();
    
    vector<vector<uint64_t>> client_data = read_csv_to_vector(address_data, data_m);
    data_m = client_data.size();
    int data_n = client_data[0].size();
    cout<<"data_m    = "<<data_m<<endl;
    cout<<"data_n    = "<<data_n<<endl;//print_data(client_data); 

    cout<<"load the client_data,                 run time is "<<(clock()-start)/1000 <<" ms"<<endl;start = clock();
    cout<<"Init fhe ... " <<endl;
    
    EncryptionParameters parms;
    parms = cdcmp_init(n,0,clr * extra);

    SEALContext* context = new SEALContext(parms);
    KeyGenerator keygen(*context);
    PublicKey pk;
    keygen.create_public_key(pk);
    SecretKey sk = keygen.secret_key();
    Encryptor *encryptor = new Encryptor(*context, pk);
    Decryptor *decryptor = new Decryptor(*context, sk);
    Evaluator *evaluator = new Evaluator(*context);
    BatchEncoder* batch_encoder = new BatchEncoder(*context);
    RelinKeys* rlk_server = new RelinKeys();
    keygen.create_relin_keys(*rlk_server);
    GaloisKeys* gal_keys_server = new GaloisKeys();
    keygen.create_galois_keys(*gal_keys_server);    
    uint64_t plain_modulus = parms.plain_modulus().value();

    //explain slot info 
    uint64_t slot_count = batch_encoder->slot_count(); // 8192 16384
    uint64_t row_count = slot_count / 2;               // 4096 8192
    uint64_t num_slots_per_element =  1;
    uint64_t num_cmps = slot_count - 1;

    cout << "Init fhe finish ... " <<endl;
    cout << "Plaintext matrix num_cmps:              " << num_cmps << endl;

    //data_m < num_cmps 
    if(num_cmps < data_m){
        cout<<"data_m : "<<data_m<<" >= num_cmps : "<<num_cmps<<endl;
        cout<<"data_m is too large, please divide different page until the size is small than num_cmps"<<endl;
        exit(0);
    }
    cout<<"Init the BFV batch scheme,            run time is "<<(clock()-start)/1000 <<" ms"<<endl;start = clock();

    vector<vector<uint64_t>> index_feat = Transpose(client_data); 
    vector<vector<Ciphertext>> client_input;
    for(int i = 0; i < index_feat.size(); i++){
        vector<vector<uint64_t>> encrypted_op_encode = rdcmp_encode_b(index_feat[i],n,slot_count,row_count);
        vector<Ciphertext> temp(n);
        Plaintext plaintext; 
        for(int i = 0 ; i < n; i++){
            batch_encoder->encode(encrypted_op_encode[i], plaintext);
            encryptor->encrypt(plaintext, temp[i]);
        }
        client_input.push_back(temp);
    }

    cout<<"encrypt the client data ,             run time is "<<(clock()-start)/1000 <<" ms"<<endl;start = clock();

    cout<<"******************************* step 1: client end *******************************"<<endl;
    //evaluater
    //client_input
    for(auto e:client_input){
        for(auto f:e){
            client_send_commun+=f.save(client_send);
        }
    }

    cout<<"******************************* step 2: server begin *******************************"<<endl;

    cout<<"load tree"<<endl;
    //string address_tree = address + "/model.json";//"../data/heart_11bits/model.json";
    Node root = Node(address_tree);
    cout<<"print tree"<<endl;
    //print_tree(root);
    cout<<"root.leaf_num()  = "<<root.leaf_num()<<endl;

    Plaintext one_one_one = init_one_one_one(batch_encoder, slot_count);
    root.value = init_zero_zero_zero_cipher(batch_encoder, encryptor, slot_count);
    root.rdcmp_pdte_init(batch_encoder,n,num_cmps,slot_count,row_count);

    cout<<"Init the tree,                          run time is "<<(clock()-start)/1000 <<" ms"<<endl;

    vector<Ciphertext> pdte_out;
    start = clock();
    pdte_rdcmp_iter(pdte_out, root, evaluator, gal_keys_server, rlk_server, client_input, one_one_one, n, batch_encoder, slot_count, row_count,num_cmps);
    
    vector<uint64_t> leaf_vec;
    leaf_extract_iter(leaf_vec, root);
    int leaf_num = leaf_vec.size();
    //for (auto e:leaf_vec){cout<<e;}cout<<endl;

    vector<Plaintext> leaf_vec_plain;//leaf_vec_plain
    for(int i = 0; i<leaf_num; i++){
        vector<uint64_t> one_zero(slot_count, leaf_vec[i]);
        Plaintext pt; 
        batch_encoder->encode(one_zero, pt);
        leaf_vec_plain.push_back(pt);
    }

    //out2
    //leaf_vec_plain

    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> distrib(1, plain_modulus - 1);
    
    vector<vector<Plaintext>> salt;//salt

    for(int i = 0; i <  2 ; i++){
        vector<Plaintext> temp;
        for(int j = 0;j < leaf_num; j++){
            temp.push_back(init_salt_salt_salt(batch_encoder,slot_count,num_cmps,distrib,gen));
        }
        salt.push_back(temp);
    }

    vector<vector<Ciphertext>> out(2, vector<Ciphertext>(0) );
    out[0] = pdte_out;
    out[1] = pdte_out;
    for(int i = 0; i < leaf_num; i++){
        evaluator->multiply_plain_inplace(out[0][i],salt[0][i]);
        evaluator->multiply_plain_inplace(out[1][i],salt[1][i]);
        evaluator->add_plain_inplace(out[1][i],leaf_vec_plain[i]);
    }

    if(clr==1){
        cout<<"clr process"<<endl;
        out = rdcmp_pdte_clear_line_relation(batch_encoder,evaluator,out,leaf_num,data_m,slot_count);
    }

    clock_t finish = clock()-start; start = clock();

    cout<<"******************************* step 2: server end *******************************"<<endl;
    //out
    for(auto e:out){
        for(auto f:e){
            server_send_commun+=f.save(server_send);
        }
    }
    cout<<"******************************* step 3: client start *******************************"<<endl;

    vector<vector<uint64_t>> ans0, ans1;
    for(int j=0;j<out[0].size();j++){
        vector<uint64_t> res0, res1;
        Plaintext pt;
        decryptor->decrypt(out[0][j], pt);
        batch_encoder->decode(pt, res0);
        decryptor->decrypt(out[1][j], pt);
        batch_encoder->decode(pt, res1);
        ans0.push_back(res0);
        ans1.push_back(res1);
    }
    vector<uint64_t> expect_result;
    for(int j = 0; j < data_m ; j++){
       for(int i = 0; i < ans0.size(); i++){
            if(ans0[i][j]==0){
                expect_result.push_back(ans1[i][j]);
                break;      
            }
        }
    }
    if(expect_result.size()<data_m){cout<<"depth_need_min is too small, please the params again by add the extra value."<<endl;exit(0);}
    
    cout<<"decrypt the result ,                 run time is "<<(clock()-start)/1000 <<"ms"<<endl;start = clock();

    cout<<"result compare"<<endl;


    for(int j = 0; j < data_m ; j++){
        uint64_t actural_result = root.eval(client_data[j]);
        //cout<< expect_result[j] <<" "<<actural_result<< endl;
        if( !( expect_result[j] == actural_result ) ){
            cout<<"In "<< j <<" col is  error "<< endl;
            cout<<expect_result[j]  <<"  "<<actural_result << endl;
            exit(0);
        }            
    }
    long comm = client_send_commun + server_send_commun;
    cout<<"compare with the real result , run time is "<<(float)(clock()-start)/1000 <<"ms"<<endl;start = clock();

    cout<<"*********************************************************************************"<<endl;
    cout<<"address_tree : "<<address_tree<<endl;
    cout<<"n            : "<<n <<endl;

    cout<<"PDTE         "<< data_m <<  "   col data ,         overall run time is "<< finish/1000 <<" ms"<<endl;
    cout<<"PDTE         "<< data_m <<  "   col data ,         overall commun.  is "<< comm/1000 <<" KB"<<endl;
    cout<<"PDTE         "<< data_m <<  "   col data ,         average run time is "<< (float)finish/1000/data_m <<" ms"<<endl;
    cout<<"PDTE         "<< data_m <<  "   col data ,         average commun.  is "<< comm/1000/data_m <<" KB"<<endl;
    cout<<"******************************* step 3: client end *******************************"<<endl;
    cout<<"all done, overall run time is "<< (clock() - global_start)/1000 <<" ms"<<endl;

    return 0;
}

