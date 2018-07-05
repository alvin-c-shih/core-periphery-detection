classdef hook_potts_cp
	methods (Access = public)
		function [C,P,Q,Qs,score,model] = detect(varargin)
			
			self = varargin{1};
			A = varargin{2};
			model = [];
			if nargin >2
				model = varargin{3};
			end
			model = self.init_model(model,A);
			
	
			C = []; P = [];Qs = [];score = []; Q = -Inf;
			for it = 1:model.itnum	
				ts = cputime;
				switch model.solver
					case 'label_switching'
						if ~isfield(model,'M')
							[Ct,Pt,Qt,Qst] = self.label_switching_potts(A,model);
						else
							[Ct,Pt,Qt,Qst] = self.label_switching(model.M,model.maxitnum);
						end
					otherwise	
						disp('unknown solver for rwcp')
				end
				
				if Qt > Q
					C = Ct;P = Pt;Q = Qt;Qs = Qst; 
				end
				
				if model.disp;disp(sprintf('%d/%d finished (max Q = %f, %f seconds)',it,model.itnum,full(Q),cputime-ts));end
			end
			
			% statistical test --------------	
			if isfield(model,'qth') 
				[C,P,Qs] = self.select_groups_by_stattest(C,P,Qs,model);
			elseif model.heuristic_selection 
				[C,P,Qs] = self.select_groups(C,P,Qs);
			end
			
			
			% re-formatting --------------
%			if isfield(model,'qth') | model.heuristic_selection	
%				Nc = sum(C,1);	
%				Np = sum(P,1);
%				noise = (Nc + Np)<=1;
%				ns = sum(C(:,noise),2) + sum(P(:,noise),2);
%				C(:,noise) = [];P(:,noise)=[];Qs(noise) = [];Nc(noise)=[];Np(noise)=[];
%			end	
			
				
			% calc other stats
			[Qs,ord] = sort(Qs,'descend');
			if ~model.isstat_test
				Nc = sum(C,1);	
				Np = sum(P,1);
				score = sum( (A*(C+P)).*C,2) - model.lambda * C*(Nc + Np -1)'/ sum(A(:));;
				C = C(:,ord);P = P(:,ord);
				Q = sum(Qs);
			end	
		end
	end
	
	
	methods (Access = private)
		
		function [C,P,Q,Qs] = label_switching_potts(self,A,model)
			
			% ******************************************* 
			% Main Routine 
			% ******************************************* 
			
			
			% --------------------------------
			% Label switching (Main routine) 
			% --------------------------------
			% --------------------------------
			% Initialise  
			% --------------------------------
			n = size(A,1);
			%[~,ord] = sort(sum(A,1),'descend');
			active = find(any(A,1));
			
			C = sparse(1:n,1:n,1);P = sparse(n,n,0);
			dQ = 1;
			loopnum = 1;
			iscore = true(n,1);
			Ac = A; Ap = sparse(n,n,0);
			Nc = ones(1,n);Np =zeros(1,n);
			while(dQ>0 & loopnum <= model.maxitnum)
				if model.disp;ts = cputime;end
					
				dQ = 0;
				ts = cputime;
				ord = randsample(active,length(active));
				for nid = ord
					% step 1
					cid = find(C(nid,:)+P(nid,:),1,'first');

					cand = find(Ac(nid,:) + Ap(nid,:));
					
					[~,lcid] = find(cid==cand);	
				
					pjgain = (Ac(nid,cand) - model.lambda* (Nc(cand)-C(nid,cand) ));%vQc;
					if( length(lcid)~=0 & iscore(nid) );pjgain(lcid) = pjgain(lcid) + model.lambda;end
					cjgain = pjgain + (Ap(nid,cand) - model.lambda * (Np(cand) - P(nid,cand)));
					if( length(lcid)~=0 & iscore(nid)==false );cjgain(lcid) = cjgain(lcid) + model.lambda;end
					lgain = 0;
					if length(lcid)~=0
						if iscore(nid)
							lgain = -cjgain(lcid);
						else
							lgain = -pjgain(lcid);
						end
						cjgain(lcid) = -Inf;pjgain(lcid) = -Inf;
					end
					
					% calculate diff of Q vals
					[v2c,v2cid] = max(cjgain);
					[v2p,v2pid] = max(pjgain);
					
					maxhitc = v2c ==cjgain;	
					maxhitp = v2p ==pjgain;	
					if sum(maxhitc)>1 
						maxcand = find(maxhitc);	
						v2cid = randsample(maxcand,1);
					end
					if sum(maxhitp)>1 
						maxcand = find(maxhitp);	
						v2pid = randsample(maxcand,1);
					end
						
					% leave gain
					v2cid = cand(v2cid);
					v2pid = cand(v2pid);
					
					% move 
					%if v2p +lgain <0 & v2c +lgain <0;continue;end
					
					if v2p==v2c & v2c + lgain >0
						if(rand()<0.5)
							v2c = v2c*2;
						else
							v2p = v2p*2;
						end
					end
				
					if v2p < v2c  & v2c + lgain >0% move to core
						if iscore(nid) % move from core
							C(nid,cid) = 0;Nc(cid) = Nc(cid)-1;
							Ac(:,cid) = Ac(:,cid) - A(:,nid);
						else % move from periphery
							P(nid,cid) = 0;Np(cid) = Np(cid)-1;
							Ap(:,cid) = Ap(:,cid) - A(:,nid); 
						end
						
						C(nid,v2cid) = 1;
						Ac(:,v2cid) = Ac(:,v2cid) + A(:,nid); 
						Nc(v2cid) = Nc(v2cid) + 1;
						dQ = dQ + v2c + lgain;
						iscore(nid) = true;
					elseif v2c < v2p & v2p + lgain >0 % move to periphery
						if iscore(nid) % move from core
							C(nid,cid) = 0;Nc(cid) = Nc(cid)-1;
							Ac(:,cid) = Ac(:,cid) - A(:,nid);
						else % move from periphery
							P(nid,cid) = 0;Np(cid) = Np(cid)-1;
							Ap(:,cid) = Ap(:,cid) - A(:,nid); 
						end
						
						P(nid,v2pid) = 1; 
						Ap(:,v2pid) = Ap(:,v2pid) + A(:,nid); 
						Np(v2pid) = Np(v2pid) + 1;
						dQ = dQ + v2p + lgain;
						iscore(nid) = false;
					end
				end
				
				
				remove = Nc ==0 & Np ==0;
				if any(remove) 
					C(:,remove)=[];P(:,remove)=[];
					Nc(remove)=[];Np(remove)=[];
					Ac(:,remove)=[];Ap(:,remove)=[];
				end

				%if model.disp;dt = cputime-ts;disp(sprintf('   %dth loop took %f seconds dQ = %f',loopnum,dt,full(dQ)));end
				loopnum = loopnum + 1;
				dt =cputime-ts;
			end
			Qs = diag( (C+P)'*A*(C+P) - P'*A*P)' - model.lambda * (Nc+Np).^2  + model.lambda * Np.^2;
			Q = sum(Qs);
		end	
		
		function [C,P] = init_CP(self,A,C,P,lambda,itnum)
		
			for i = 1:itnum	
				Nc = sum(C,1);
				Np = sum(P,1);
			
				
				% calc vc
				[r,c,v] = find(A*C);
				v = v - lambda * Nc(c)'-C(sub2ind(size(C),r,c));
				vQc = sparse(r,c,v,size(A,1),size(C,2));
					
				% calc vp
				[r,c,v] = find(A*P);
				v = v - lambda * Np(c)'-P(sub2ind(size(P),r,c));
				vQp = sparse(r,c,v,size(A,1),size(C,2));
				
				% calc gain
				pjgain = 2*vQc;
				cjgain = pjgain + 2*vQp;
				lgain = -sum(cjgain.*C,2) -sum(pjgain.*P,2);
				
				% find maximum	
				[v2p,v2pid] = max(pjgain');
				[v2c,v2cid] = max(cjgain');
				
				% move to core
				moveC = (v2p' <= v2c') & (0 <v2c'+lgain) ;
				moveP = (v2c' < v2p') & (0<v2p'+lgain) ;
				C(moveC | moveP,:) = 0;
				P(moveC | moveP,:) = 0;
				C = C + sparse( find(moveC),v2cid(moveC),1,size(C,1),size(C,2));
				P = P + sparse( find(moveP),v2pid(moveP),1,size(P,1),size(P,2));
				
				% remove empty  	
				remove = ~any(C+P,1);
				C(:,remove) = []; P(:,remove)=[];
			end	
			%vQc = Ac(nid,:) - model.lambda * (Nc-C(nid,:));
			%vQp = Ap(nid,:) - model.lambda * (Np-P(nid,:));
			
		end
	
			
		function [C,P,Q,Qs,score] = label_switching(self,M,maxitnum)
			
			% ******************************************* 
			% Main Routine 
			% ******************************************* 
			
			% --------------------------------
			% Initialise  
			% --------------------------------
			
			% --------------------------------
			% Label switching (Main routine) 
			% --------------------------------
			n = size(M,1);
			C = sparse(1:n,1:n,1);P = sparse(n,n,0);
			dQ = 1;
			loopnum = 1;
			while(dQ>0 & loopnum <= maxitnum)
				dQ = 0;
				ord = randsample(n,n)';
				for nid =ord
					iscore = any(C(nid,:));
					cQv = (C'*M(:,nid))';	
					pQv = (P'*M(:,nid))';	
					vQc = M(nid,:)*C;
					vQp = M(nid,:)*P;
					vQv = M(nid,nid);
					
					% core join gain 
					cjgain = cQv + pQv + vQc + vQp + vQv;
					
					% periphery join gain 
					pjgain = cQv + pQv + vQc + vQp -vQp - pQv;
					% calculate diff of Q vals
					[v2c,v2cid] = max(cjgain);
					[v2p,v2pid] = max(pjgain);
					
					if iscore
						% leave gain
						cid = find(C(nid,:)>0);
						lgain = -cQv - pQv - vQc - vQp + vQv;
						lgain = lgain(cid);
					else
						% leave gain
						cid = find(P(nid,:)>0);
						lgain = -cQv - pQv - vQc - vQp + vQp + pQv;
						lgain = lgain(cid);
					end
					if v2p < v2c & 0 <= v2c + lgain & cid ~=v2cid
						C(nid,:) = 0; 
						P(nid,:) = 0; 
						C(nid,v2cid) = 1; 
						dQ = dQ + v2c + lgain;
						updated = true;
					elseif v2c <= v2p & 0 <= v2p + lgain & cid ~=v2pid
						C(nid,:) = 0; 
						P(nid,:) = 0; 
						P(nid,v2pid) = 1; 
						dQ = dQ + v2p + lgain;
						updated = true;
					end
				end	
				loopnum = loopnum + 1;
			end
			
			slice = any(C+P,1);
			C = C(:,slice);
			P = P(:,slice);
			Nc = sum(C,1); Np = sum(P,1);
			score = sum( (M*(C+P)).*C,2);
			Qs = diag( (C+P)'*M*(C+P) - P'*M*P)';
			[Qs,ord] = sort(Qs,'descend');
			C = C(:,ord);P = P(:,ord);
			Q = sum(Qs);
		end
		
		function Alist = ergraph(self,rho,N,T)
			M = N*(N-1)/2;
			L = nchoosek(1:N,2);
			t = 1;
			Alist = cell(T,1);	
			while(t <=T)	
				m = binornd(M,full(rho));
			
				eidx = randsample(M,m,false);
				A = sparse(L(eidx,1),L(eidx,2),1,N,N);
				A = A + A'; 
				Alist{t,1} = A;
				t = t + 1;
			end
		end

		function [C,P,Qs] = select_groups(self,C,P,Qs)
			[Qs,ord] = sort(Qs,'descend');	
			C = C(:,ord); P = P(:,ord);
			[~,idx] = max(-diff([Qs,0]));
			C = C(:,1:idx); P = P(:,1:idx);
			Qs = Qs(1:idx);	
		end
	
		function model = init_model(self,model,A)
			
			if ~isfield(model,'lambda');model.lambda = graph(A).density();end
			if ~isfield(model,'itnum');model.itnum = 10;end
			if ~isfield(model,'disp');model.disp = false;end
			if ~isfield(model,'isstat_test');model.isstat_test = false;end
			if ~isfield(model,'maxitnum');model.maxitnum = 80;end
			if ~isfield(model,'solver');model.solver = 'label_switching';end
			if ~isfield(model,'heuristic_selection');model.heuristic_selection =false;end
			
			if isfield(model,'name2');
				if strcmp(model.name2,'config') 
					model.M = graph(A).modularity_matrix();
				end
			end
		end
		function [C,P,Qs,model] = select_groups_by_stattest(self,C,P,Qs,model)
			slice = false(1,length(Qs));
			[Qs,ord] = sort(Qs,'descend');
			C = C(:,ord); P=P(:,ord);
			for i = 1:length(Qs)
				slice(i) = Qs(i) >= model.qth;
			end
			C = C(:,slice); P = P(:,slice); Qs = Qs(slice); Q = sum(Qs);						
		end
	end
end
