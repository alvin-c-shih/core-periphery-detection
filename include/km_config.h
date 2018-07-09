/*
*
* Header file of the KM-config algorithm (C++ version)
*
*
* An algorithm for finding multiple core-periphery pairs in networks
*
*
* Core-periphery structure requires something else in the network
* Sadamori Kojaku and Naoki Masuda
* Preprint arXiv:1710.07076
* 
*
* Please do not distribute without contacting the authors.
*
*
* AUTHOR - Sadamori Kojaku
*
*
* DATE - 11 Oct 2017
*/
#ifndef CP_ALGORITHM 
#define CP_ALGORITHM
	#include "cpalgorithm.h" 
#endif

class KM_config: public CPAlgorithm{
public:
	// Constructor 
	KM_config();
	KM_config(int num_runs);

	// function needed to be implemented

	void detect(const Graph& G);
	
	void calc_Q(
	    const Graph& G,
	    const vector<int>& c,
	    const vector<bool>& x,
	    double& Q,
	    vector<double>& q);
	
protected: 
	
private:
	int _num_runs;
	uniform_real_distribution<double> _udist;
	
	void _km_config_label_switching(
	    const Graph& G,
	    const int num_of_runs,
	    vector<int>& c,
	    vector<bool>& x,
	    double& Q,
	    vector<double>& q,
            mt19937_64& mtrnd
		);

	double _calc_dQ_conf(double d_i_c,
	    double d_i_p,
	    double d_i,
	    double D_c,
	    double D_p,
	    double selfloop,
	    bool x,
	    const double M);
	
	void _propose_new_label(
	    const Graph& G,
	    const vector<int>& c,
	    const vector<bool>& x,
	    const vector<double>& sum_of_deg_core,
	    const vector<double>& sum_of_deg_peri,
	    const double M,
	    const int node_id,
	    int& cprime,
	    bool& xprime,
	    double& dQ,
	    mt19937_64& mtrnd
	    );
	
	
	void _km_config_label_switching_core(
	    const Graph& G,
	    vector<int>& c,
	    vector<bool>& x,
	    mt19937_64& mtrnd
	    );
};


/*-----------------------------
Constructor
-----------------------------*/
KM_config::KM_config(int num_runs):CPAlgorithm(){
	KM_config();
	_num_runs = num_runs;
};

KM_config::KM_config():CPAlgorithm(){
	uniform_real_distribution<double> tmp(0.0,1.0);
	_udist = tmp;
	_num_runs = 10;
	_mtrnd = _init_random_number_generator();
};


/*-----------------------------
Functions inherited from the super class (CPAlgorithm)
-----------------------------*/
void KM_config::detect(const Graph& G){
	_km_config_label_switching(G, _num_runs, _c, _x, _Q, _q, _mtrnd);
}

void KM_config::calc_Q(
    const Graph& G,
    const vector<int>& c,
    const vector<bool>& x,
    double& Q,
    vector<double>& q)
{
    int N = G.get_num_nodes();
    int K = *max_element(c.begin(), c.end()) + 1;
    q.assign(K, 0.0);
    vector<double> Dc(K);
    vector<double> Dp(K);
    fill(Dc.begin(), Dc.end(), 0.0);
    fill(Dp.begin(), Dp.end(), 0.0);

    double double_M = 0.0;
    for (int i = 0; i < N; i++) {
	int sz = G.degree(i);
	double di = 0;
        for (int j = 0; j < sz; j++) {
	    Neighbour nn = G.get_kth_neighbour(i, j);
	    int nei = nn.get_node();
	    double wj = nn.get_w();
            q[c[i]] += wj * !!(c[i] == c[nei]) * !!(x[i] | x[nei]);
	    di+=wj;
        }
        Dc[c[i]] += !!(x[i]) * di;
        Dp[c[i]] += !!(!x[i]) * di;
        double_M += di;
    }
    Q = 0;
    for (int k = 0; k < K; k++) {
        q[k] = (q[k] - (Dc[k] * Dc[k] + 2 * Dc[k] * Dp[k]) / double_M) / double_M;
        Q += q[k];
    }
}

/*-----------------------------
Private functions (internal use only)
-----------------------------*/

/* 
* Use this function instead if you have problem in initialising mtrnd
* This sometimes happens when the version of gcc compiler is different from the one required by mex compiler.
*/ 

void KM_config::_km_config_label_switching(
    const Graph& G,
    const int num_of_runs,
    vector<int>& c,
    vector<bool>& x,
    double& Q,
    vector<double>& q,
    mt19937_64& mtrnd
    )
{

    Q = -1;
    for (int i = 0; i < num_of_runs; i++) {
        vector<int> ci;
        vector<bool> xi;
        vector<double> qi;
        double Qi = 0.0;

        _km_config_label_switching_core(G, ci, xi, mtrnd);

        calc_Q(G, ci, xi, Qi, qi);
	
        if (Qi > Q) {
            c = ci;
            x = xi;
            Q = Qi;
            q = qi;
        }
    }
}


double KM_config::_calc_dQ_conf(double d_i_c,
    double d_i_p,
    double d_i,
    double D_c,
    double D_p,
    double selfloop,
    bool x,
    const double M)
{
    return 2 * (d_i_c + d_i_p * (!!(x)) - d_i * (D_c + D_p * !!(x)) / (2.0 * M)) + !!(x) * (selfloop - d_i * d_i / (2.0 * M));
}


void KM_config::_propose_new_label(
    const Graph& G,
    const vector<int>& c,
    const vector<bool>& x,
    const vector<double>& sum_of_deg_core,
    const vector<double>& sum_of_deg_peri,
    const double M,
    const int node_id,
    int& cprime,
    bool& xprime,
    double& dQ,
    mt19937_64& mtrnd)
{
    int N = G.get_num_nodes();
    int neighbourNum = G.degree(node_id);

    double deg = G.wdegree(node_id);

    vector<double> edges_to_core(N);
    vector<double> edges_to_peri(N);

    fill(edges_to_core.begin(), edges_to_core.end(), 0.0);
    fill(edges_to_peri.begin(), edges_to_peri.end(), 0.0);
    double selfloop = 0;
    for (int j = 0; j < neighbourNum; j++) {
	Neighbour nn = G.get_kth_neighbour(node_id, j);
	int nei = nn.get_node();
	double wj = nn.get_w();
	
	if(node_id == nei){
		selfloop+= 1;
		continue;
	}
	
        edges_to_core[c[nei]] += wj * (double)!!(x[nei]);
        edges_to_peri[c[nei]] += wj * (double)!!(!x[nei]);
    }

    double D_core = sum_of_deg_core[c[node_id]] - deg * (double)!!(x[node_id]);
    double D_peri = sum_of_deg_peri[c[node_id]] - deg * (double)!!(!x[node_id]);
    double dQold = _calc_dQ_conf(edges_to_core[c[node_id]], edges_to_peri[c[node_id]], deg,
        D_core, D_peri, selfloop, x[node_id], M);

    dQ = 0;
    for (int j = 0; j < neighbourNum; j++) {
	Neighbour nn = G.get_kth_neighbour(node_id, j);
	int nei = nn.get_node();
	//double wj = nn.get_w();

        int cid = c[nei];

        D_core = sum_of_deg_core[cid] - deg * (double)!!( (c[node_id] == cid) & x[node_id]);
        D_peri = sum_of_deg_peri[cid] - deg * (double)!!( (c[node_id] == cid) & !x[node_id]);

        double Q_i_core = _calc_dQ_conf(edges_to_core[cid], edges_to_peri[cid],
            deg, D_core, D_peri, selfloop, true, M);
        double Q_i_peri = _calc_dQ_conf(edges_to_core[cid], edges_to_peri[cid],
            deg, D_core, D_peri, selfloop, false, M);
        Q_i_core -= dQold;
        Q_i_peri -= dQold;

        if (MAX(Q_i_core, Q_i_peri) < dQ)
            continue;

        if (Q_i_peri < Q_i_core) {
            xprime = true;
            cprime = cid;
            dQ = Q_i_core;
        }
        else if (Q_i_peri > Q_i_core) {
            xprime = false;
            cprime = cid;
            dQ = Q_i_peri;
        }
        else {
            cprime = cid;
            xprime = _udist(mtrnd) < 0.5;
            dQ = Q_i_core;
        }
    }
}

void KM_config::_km_config_label_switching_core(
    const Graph& G,
    vector<int>& c,
    vector<bool>& x,
    mt19937_64& mtrnd
    )
{
    /* Variable declarations */
    int N = G.get_num_nodes();
    vector<double> sum_of_deg_core(N);
    vector<double> sum_of_deg_peri(N);
    vector<int> order(N);
    vector<double> degs(N);
    double M = 0;
    bool isupdated = false;
    fill(sum_of_deg_core.begin(), sum_of_deg_core.end(), 0.0);
    fill(sum_of_deg_peri.begin(), sum_of_deg_peri.end(), 0.0);
    c.clear();
    x.clear();
    c.assign(N, 0);
    x.assign(N, true);
    for (int i = 0; i < N; i++) {
        order[i] = i;
        c[i] = i;
        double deg = G.wdegree(i);
	degs[i] = deg;
        sum_of_deg_core[i] += (double)!!(x[i]) * deg;
        M += deg;
    };
    M = M / 2;

    /* Label switching algorithm */
    do {
        isupdated = false;
        shuffle(order.begin(), order.end(), mtrnd);

        for (int scan_count = 0; scan_count < N; scan_count++) {
            int i = order[scan_count];

            int cprime = c[i]; // c'
            bool xprime = x[i]; // x'

            double dQ = 0;
            _propose_new_label(G, c, x, sum_of_deg_core, sum_of_deg_peri,
                M, i, cprime, xprime, dQ, mtrnd);

            if (dQ <= 0)
                continue;
		
            if ( (c[i] == cprime) & (x[i] == xprime) )
                continue;

            double deg = degs[i];
            if (x[i]) {
                sum_of_deg_core[c[i]] -= deg;
            }
            else {
                sum_of_deg_peri[c[i]] -= deg;
            }

            if (xprime) {
                sum_of_deg_core[cprime] += deg;
            }
            else {
                sum_of_deg_peri[cprime] += deg;
            }

            c[i] = cprime;
            x[i] = xprime;

            isupdated = true;
        }

    } while (isupdated == true);

    /* Remove empty core-periphery pairs */
    std::vector<int> labs;
    for (int i = 0; i < N; i++) {
        int cid = -1;
	int labsize = labs.size();
        for (int j = 0; j < labsize; j++) {
            if (labs[j] == c[i]) {
                cid = j;
                break;
            }
        }

        if (cid < 0) {
            labs.push_back(c[i]);
            cid = labs.size() - 1;
        }
        c[i] = cid;
    }
}

/* Louvain algorithm */

void KM_config::_km_modmat_louvain_core(
	const Graph& G, 
    	vector<int>& c,
    	vector<bool>& x,
        mt19937_64& mtrnd
	){

	// Intiialise variables	
	int N = G.get_num_nodes();
	c.clear();
	x.clear();
    	c.assign(N, 0); 
    	x.assign(N, true);
    	for (int i = 0; i < N; i++) c[i] = i;
	
	vector<int>ct = c; // label of each node at tth iteration
	vector<bool>xt = x; // label of each node at tth iteration. 
	Graph cnet_G; // coarse network
	vector<int> toLayerId; //toLayerId[i] maps 2*c[i] + x[i] to the id of node in the coarse network 
	_coarsing(G, ct, xt, cnet_G, toLayerId); // Initialise toLayerId

	double Qbest = 0; // quality of the current partition

	int cnet_N;
	do{
		cnet_N = cnet_G.get_num_nodes();
		
		// Core-periphery detection	
		vector<int> cnet_c; // label of node in the coarse network, Mt 
		vector<bool> cnet_x; // label of node in the coarse network, Mt 
		_km_modmat_label_switching_core(cnet_G, cnet_c, cnet_x, mtrnd);
	
		// Update the label of node in the original network, ct and xt.	
		for(int i = 0; i< N; i++){
			int cnet_id = toLayerId[2 * ct[i] + xt[i]];
			ct[i] = cnet_c[ cnet_id ];
			xt[i] = cnet_x[ cnet_id ];
		}
 		
		// Compute the quality       	
		double Qt = 0; vector<double> qt;
		calc_Q_config(cnet_G, cnet_c, cnet_x, Qt, qt);

		if(Qt>=Qbest){ // if the quality is the highest among those detected so far
			c = ct;
			x = xt;
			Qbest = Qt;
		}
	
		// Coarsing	
		vector<vector<double>> new_cnet_G; 
		_coarsing(cnet_G, cnet_c, cnet_x, new_cnet_G, toLayerId);
		cnet_G = new_cnet_G;
		
		int sz = cnet_G.get_num_nodes();
		if(sz == cnet_N) break;	
			
	}while( true );

	_relabeling(c);
}

void KM_config::_km_config_louvain(
    const Graph& G,
    const int num_of_runs,
    vector<int>& c,
    vector<bool>& x,
    double& Q,
    vector<double>& q,
    mt19937_64& mtrnd
    )
{

    int N = G.get_num_nodes();
    c.clear();
    x.clear();
    c.assign(N, 0);
    x.assign(N, true);

    /* Generate \hat q^{(s)} and \hat n^{(s)} (1 \leq s \leq S) */
    // create random number generator per each thread
    int numthread = 1;
    #ifdef _OPENMP
    	# pragma omp parallel
    	{
    		numthread = omp_get_num_threads();
    	}
    #endif
    vector<mt19937_64> mtrnd_list(numthread);
    for(int i = 0; i < numthread; i++){
	mt19937_64 mtrnd = _init_random_number_generator();
	mtrnd_list[i] = mtrnd;
    }

    Q = -1;
    #ifdef _OPENMP
    #pragma omp parallel for shared(c, x, Q, q, N, G, mtrnd_list)
    #endif
    for (int i = 0; i < num_of_runs; i++) {
        vector<int> ci;
        vector<bool> xi;
        vector<double> qi;
        double Qi = 0.0;

        int tid = 0;
    	#ifdef _OPENMP
        	tid = omp_get_thread_num();
    	#endif
	
        mt19937_64 mtrnd = mtrnd_list[tid];
        _km_config_louvain_core(G, ci, xi, mtrnd);

        calc_Q_config(G, ci, xi, Qi, qi);

        #pragma omp critical
        {
        	if (Qi > Q) {
		    for(int i = 0; i < N; i++){
			c[i] = ci[i];
			x[i] = xi[i];
		    }
		    q.clear();
		    int K = qi.size();
	            vector<double> tmp(K,0.0);	
		    q = tmp;
		    for(int k = 0; k < K; k++){
			q[k] = qi[k];
		    }
        	    Q = Qi;
        	}
	}
    }
}

void KM_config::_coarsing(
    	const Graph& G,
    	const vector<int>& c,
    	const vector<bool>& x,
    	Graph& newG,
    	vector<int>& toLayerId 
	){
		
        int N = c.size();
	vector<int> ids(N,0);
    	int maxid = 0;
	for(int i = 0;i<N;i++){
		ids[i] = 2 * c[i] + x[i];
		maxid = MAX(maxid, ids[i]);
	}
	_relabeling(ids);
	toLayerId.clear();
	toLayerId.assign(maxid+1,0);
	for(int i = 0;i<N;i++){
		toLayerId[2 * c[i] + x[i]] = ids[i];
	}
	
	
    	int K = *max_element(ids.begin(), ids.end()) + 1;
	newG = Graph(K);

	for(int i = 0;i<N;i++){
		int mi = 2 * c[i] + x[i];
		for(int j = 0;j<N;j++){
			int mj = 2 * c[j] + x[j];
			Neighbour nb = G.get_kth_neighbour(i, j);
			int sid = toLayerId[mi];
			int did = toLayerId[mj];
			double w = nb.get_w();
			newG.addEdge(sid, did, w);
		}
	}
	
	newG.compress();
}

int KM_config::_count_non_empty_block(
    	vector<int>& c,
    	vector<bool>& x
	){
	int N = c.size();
	vector<int> ids(N,0);
	for(int i = 0; i< N; i++){
		ids[i] = 2 * c[i] + x[i];
	}
	sort(ids.begin(), ids.end());
	return unique(ids.begin(), ids.end()) - ids.begin();
}
