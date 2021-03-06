import _cpalgorithm as _cp
from scipy.sparse import diags
from scipy.sparse.linalg import eigs
from .CPAlgorithm import * 

class LowRankCore(CPAlgorithm):
	"""LowRankCore algorithm.
	
	LowRankCore algorithm introduced in Ref.~ [1]
	
	Parameters
	----------
	beta : float
		Minimum fraction of core or peripheral nodes.
		This parameter ensures :math:`\\beta \\leq \\frac{Nc}{N_c + N_p}, \\frac{Np}{N_c + N_p}`, where
		:math:`N_c` and  :math:`N_p` are the number of core and peripheral nodes, respectively.  (optional, default: 0.1)
		
	Examples
	--------
	Create this object.

	>>> import cpalgorithm as cpa	
	>>> lrc = cpa.LowRankCore()
	
	**Core-periphery detection**
	
	Detect core-periphery structure in network G (i.e., NetworkX object):
	
	>>> lrc.detect(G) 
	
	Retrieve the ids of the core-periphery pair to which each node belongs:
	
	>>> pair_id = lrc.get_pair_id() 
	
	Retrieve the coreness:

	>>> coreness = lrc.get_coreness() 
		
	.. note::

	   This algorithm can accept unweighted and weighted networks.
	   The algorithm assigns all nodes into the same core-periphery pair by construction, i.e., c[node_name] =0 for all node_name.

	.. rubric:: Reference

	[1] M. Cucuringu, P. Rombach, S. H. Lee, and M. A. Porter Detection of core-periphery structure in networks using spectral methods and geodesic paths. Euro. J. Appl. Math., 27:846–887, 2016.

	"""
	
	
	def __init__(self, beta = 0.1):
		self.beta = beta
	
	def detect(self, G):
		"""Detect a single core-periphery pair.
	
		Parameters
		----------
		G : NetworkX graph object
		
		Examples
		--------
		>>> import networkx as nx
		>>> import cpalgorithm as cpa
		>>> G = nx.karate_club_graph()  # load the karate club network. 
		>>> lrc = cp.LowRankCore()
		>>> lrc.detect(G)

		"""
		
		self.c_, self.x_ = self._low_rank_core(G)
		
		self.Q_ = self._score(G, self.c_, self.x_) 
		self.qs_ = self.Q_
	
	def _score(self, G, c, x):
		N = nx.number_of_nodes(G)
		A = nx.to_scipy_sparse_matrix(G)
		
		xx = np.zeros((N,1))
		for idx, nd in enumerate(G.nodes()):	
			xx[idx] = x[nd]
			
		Mcc = np.dot(xx.T * A, xx)/2
		Mcp = np.dot(xx.T * A, (1-xx))
		Mpp = np.dot(xx.T * A, xx)/2
		i = np.sum(xx)
		if i <2 or i >N-2:
			return [0.0]
 
		q = Mcc/float(i*(i-1)/2) + Mcp/float(i*(N-i)) - Mpp / float((N-i)*((N-i)-1)/2)
		return [q[0,0]]

	def _find_cut(self, G, score, b):
		node_pairs, w, node2id, id2node = self._to_edge_list(G)

		N = nx.number_of_nodes(G)
		M = nx.number_of_edges(G) 
		A = nx.to_scipy_sparse_matrix(G)
		qc = np.zeros(N)
		qp = np.zeros(N)
		od = (-score).argsort()

		for i in range(b, N-b):
			x = np.zeros((N,1))
			x[od[0:i]] = 1
			
			Mcc = np.dot(x.T * A, x)[0,0]/2
			Mcp = np.dot(x.T * A, (1-x))[0,0]
			Mpp = np.dot((1-x).T * A, (1-x))[0,0]/2
			qc[i] = Mcc/float(i*(i-1)/2) + Mcp/float(i*(N-i)) - Mpp / float((N-i)*((N-i)-1)/2)
			qp[i] = Mcp/float(i*(N-i)) + Mpp / float((N-i)*((N-i)-1)/2) - Mcc/float(i*(i-1)/2)
 
		idx_c = np.argmax(qc)
		idx_p = np.argmax(qp)
	
		if qc[idx_c] > qp[idx_p]:
			Q = qc[idx_c]
			x = np.zeros(N)
			x[od[0:idx_c]] = 1
		else:
			Q = qc[idx_p]
			x = np.ones(N)
			x[od[0:idx_p]] = 0
			
		Q = Q/N
		c = dict(zip( [id2node[i] for i in range(N)], np.zeros(N).astype(int)))
		x = dict(zip( [id2node[i] for i in range(N)], x.astype(int).tolist()))
		return c, x

	def _low_rank_core(self, G):

		node_pairs, w, node2id, id2node = self._to_edge_list(G)

		N = nx.number_of_nodes(G)
		M = nx.number_of_edges(G) 
		A = nx.to_scipy_sparse_matrix(G).asfptype()
		
		d, v = eigs(A, k=2, which='LM');	
	
		At = (np.dot(v * diags(d), v.T)>0.5).astype(int)
		score = At.sum(axis=0)
	
		c, x = self._find_cut(G, score, int(np.round(N * self.beta)) )
		return c, x
		
class LapCore(CPAlgorithm):
	"""LapCore algorithm.
	
	LapCore algorithm introduced in Ref.~ [1]
	
	Parameters
	----------
	beta : float
		Minimum fraction of core or peripheral nodes.
		This parameter ensures :math:`\\beta \\leq \\frac{Nc}{N_c + N_p}, \\frac{Np}{N_c + N_p}`, where
		:math:`N_c` and  :math:`N_p` are the number of core and peripheral nodes, respectively.  (optional, default: 0.1)
		
	Examples
	--------
	Create this object.

	>>> import cpalgorithm as cpa	
	>>> lc = cpa.LapCore()
	
	**Core-periphery detection**
	
	Detect core-periphery structure in network G (i.e., NetworkX object):
	
	>>> lc.detect(G) 
	
	Retrieve the ids of the core-periphery pair to which each node belongs:
	
	>>> pair_id = lc.get_pair_id() 
	
	Retrieve the coreness:

	>>> coreness = lc.get_coreness() 
		
	.. note::

	   This algorithm can accept unweighted and weighted networks.
	   Also, the algorithm assigns all nodes into the same core-periphery pair by construction, i.e., c[node_name] =0 for all node_name.

	.. rubric:: Reference

	[1] M. Cucuringu, P. Rombach, S. H. Lee, and M. A. Porter Detection of core-periphery structure in networks using spectral methods and geodesic paths. Euro. J. Appl. Math., 27:846–887, 2016.

	"""
	
	
	def __init__(self, beta = 0.1):
		self.beta = beta
	
	def detect(self, G):
		"""Detect a single core-periphery pair.
	
		Parameters
		----------
		G : NetworkX graph object
		
		Examples
		--------
		>>> import networkx as nx
		>>> import cpalgorithm as cpa
		>>> G = nx.karate_club_graph()  # load the karate club network. 
		>>> lc = cp.LapCore()
		>>> lc.detect(G)

		"""
		
		self.c_, self.x_ = self._lap_core(G)
		
		self.Q_ = self._score(G, self.c_, self.x_) 
		self.qs_ = self.Q_
	
	def _score(self, G, c, x):
		N = nx.number_of_nodes(G)
		A = nx.to_scipy_sparse_matrix(G)
		
		xx = np.zeros((N,1))
		for idx, nd in enumerate(G.nodes()):	
			xx[idx] = x[nd]
			
		Mcc = np.dot(xx.T * A, xx)/2
		Mcp = np.dot(xx.T * A, (1-xx))
		Mpp = np.dot(xx.T * A, xx)/2
		i = np.sum(xx)
		if i <2 or i >N-2:
			return [0.0]
 
		q = Mcc/float(i*(i-1)/2) + Mcp/float(i*(N-i)) - Mpp / float((N-i)*((N-i)-1)/2)
		return [q[0,0]]

	
	def _find_cut(self, G, score, b):
		node_pairs, w, node2id, id2node = self._to_edge_list(G)

		N = nx.number_of_nodes(G)
		M = nx.number_of_edges(G) 
		A = nx.to_scipy_sparse_matrix(G)
		qc = np.zeros(N)
		qp = np.zeros(N)
		od = (-score).argsort()

		for i in range(b, N-b):
			x = np.zeros((N,1))
			x[od[0:i]] = 1
			
			Mcc = np.dot(x.T * A, x)[0,0]/2
			Mcp = np.dot(x.T * A, (1-x))[0,0]
			Mpp = np.dot((1-x).T * A, (1-x))[0,0]/2
			qc[i] = Mcc/float(i*(i-1)/2) + Mcp/float(i*(N-i)) - Mpp / float((N-i)*((N-i)-1)/2)
			qp[i] = Mcp/float(i*(N-i)) + Mpp / float((N-i)*((N-i)-1)/2) - Mcc/float(i*(i-1)/2)
 
		idx_c = np.argmax(qc)
		idx_p = np.argmax(qp)
	
		if qc[idx_c] > qp[idx_p]:
			Q = qc[idx_c]
			x = np.zeros(N)
			x[od[0:idx_c]] = 1
		else:
			Q = qc[idx_p]
			x = np.ones(N)
			x[od[0:idx_p]] = 0
			
		Q = Q/N
		c = dict(zip( [id2node[i] for i in range(N)], np.zeros(N).astype(int)))
		x = dict(zip( [id2node[i] for i in range(N)], x.astype(float).tolist()))
		return c, x

	def _lap_core(self, G):

		node_pairs, w, node2id, id2node = self._to_edge_list(G)

		N = nx.number_of_nodes(G)
		M = nx.number_of_edges(G) 
		A = nx.to_scipy_sparse_matrix(G)
		deg = np.array([d[1] for d in G.degree()])
		denom =  np.zeros(N)
		denom[deg>0] = 1.0 / (deg[deg>0] + 1.0)
		T = diags(denom) * A -diags(np.ones(N))
		d, v = eigs(T, k=1, which='SR')
		
		c, x = self._find_cut(G, v.T[0], int(np.round(N * self.beta)) )
		return c, x

class LapSgnCore(CPAlgorithm):
	"""LowSgnCore algorithm.
	
	LapSgnCore algorithm introduced in Ref.~ [1]
		
	Examples
	--------
	Create this object.

	>>> import cpalgorithm as cpa	
	>>> lsc = cpa.LapSgnCore()
	
	**Core-periphery detection**
	
	Detect core-periphery structure in network G (i.e., NetworkX object):
	
	>>> lsc.detect(G) 
	
	Retrieve the ids of the core-periphery pair to which each node belongs:
	
	>>> pair_id = lsc.get_pair_id() 
	
	Retrieve the coreness:

	>>> coreness = lsc.get_coreness() 
		
	.. note::

	   This algorithm can accept unweighted and weighted networks.
	   Also, the algorithm assigns all nodes into the same core-periphery pair by construction, i.e., c[node_name] =0 for all node_name.

	.. rubric:: Reference

	[1] M. Cucuringu, P. Rombach, S. H. Lee, and M. A. Porter Detection of core-periphery structure in networks using spectral methods and geodesic paths. Euro. J. Appl. Math., 27:846–887, 2016.

	"""
	
	def __init__(self):
		self.beta = 0.1
	
	def detect(self, G):
		"""Detect a single core-periphery pair.
	
		Parameters
		----------
		G : NetworkX graph object
		
		Examples
		--------
		>>> import networkx as nx
		>>> import cpalgorithm as cpa
		>>> G = nx.karate_club_graph()  # load the karate club network. 
		>>> lsc = cp.LapSgnCore()
		>>> lsc.detect(G)

		"""
		
		self.c_, self.x_ = self._lapsgn_core(G)
		
		self.Q_ = self._score(G, self.c_, self.x_) 
		self.qs_ = self.Q_
	
	def _score(self, G, c, x):
		N = nx.number_of_nodes(G)
		A = nx.to_scipy_sparse_matrix(G)
		
		xx = np.zeros((N,1))
		for idx, nd in enumerate(G.nodes()):	
			xx[idx] = x[nd]
			
		Mcc = np.dot(xx.T * A, xx)/2
		Mcp = np.dot(xx.T * A, (1-xx))
		Mpp = np.dot(xx.T * A, xx)/2
		i = np.sum(xx)
		if i <2 or i >N-2:
			return [0.0]
 
		q = Mcc/float(i*(i-1)/2) + Mcp/float(i*(N-i)) - Mpp / float((N-i)*((N-i)-1)/2)
		return [q[0,0]]

	def _lapsgn_core(self, G):
		node_pairs, w, node2id, id2node = self._to_edge_list(G)

		N = nx.number_of_nodes(G)
		M = nx.number_of_edges(G) 
		A = nx.to_scipy_sparse_matrix(G)
		deg = np.array([d[1] for d in G.degree()])
		denom =  np.zeros(N)
		denom[deg>0] = 1.0 / (deg[deg>0] + 1.0)
		T = diags(denom) * A -diags(np.ones(N))
		d, v = eigs(T, k=1, which='SR');
		v = np.sign(v);
	
		c = dict(zip( [id2node[i] for i in range(N)], np.zeros(N).astype(int)))
		xp = dict(zip( [id2node[i] for i in range(N)], (v.T>0).astype(float).tolist()[0]))
		xn = dict(zip( [id2node[i] for i in range(N)], (v.T<0).astype(float).tolist()[0]))
		if self._score(G, c, xn) < self._score(G, c, xp):
			x = xp
		else:
			x = xn

		return c, x	
