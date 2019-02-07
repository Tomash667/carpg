#pragma once

//-----------------------------------------------------------------------------
// Kahn's algorithm, check if graph have loops
class Graph
{
public:
	struct Edge
	{
		int from;
		int to;
	};

	Graph(int vertices) : vertices(vertices)
	{
	}

	void AddEdge(int from, int to)
	{
		edges.push_back({ from, to });
	}

	bool Sort()
	{
		vector<int> S;

		for(int i = 0; i < vertices; ++i)
		{
			bool any = false;
			for(auto& e : edges)
			{
				if(e.to == i)
				{
					any = true;
					break;
				}
			}
			if(!any)
				S.push_back(i);
		}

		while(!S.empty())
		{
			int n = S.back();
			S.pop_back();
			result.push_back(n);

			for(auto it = edges.begin(), end = edges.end(); it != end; )
			{
				if(it->from == n)
				{
					int m = it->to;
					it = edges.erase(it);
					end = edges.end();

					bool any = false;
					for(auto& e : edges)
					{
						if(e.to == m)
						{
							any = true;
							break;
						}
					}
					if(!any)
						S.push_back(m);
				}
				else
					++it;
			}
		}

		// if there any edges left, graph has cycles
		return edges.empty();
	}

	vector<int>& GetResult() { return result; }

private:
	vector<int> result;
	vector<Edge> edges;
	int vertices;
};

//-----------------------------------------------------------------------------
// Tree iterator
template<typename T>
struct TreeItem
{
	struct Iterator
	{
	private:
		T* current;
		int index, depth;
		bool up;

	public:
		Iterator(T* current, bool up) : current(current), index(-1), depth(0), up(up) {}

		bool operator != (const Iterator& it) const
		{
			return it.current != current;
		}

		void operator ++ ()
		{
			while(current)
			{
				if(index == -1)
				{
					if(current->childs.empty())
					{
						index = current->index;
						current = current->parent;
						--depth;
					}
					else
					{
						index = -1;
						current = current->childs.front();
						++depth;
						break;
					}
				}
				else
				{
					if(index == current->childs.size())
					{
						index = current->index;
						current = current->parent;
						--depth;
					}
					else if(index + 1 == current->childs.size())
					{
						if(up)
						{
							++index;
							break;
						}
						else
						{
							index = current->index;
							current = current->parent;
							--depth;
						}
					}
					else
					{
						current = current->childs[index + 1];
						index = -1;
						++depth;
						break;
					}
				}
			}
		}

		T* GetCurrent() { return current; }
		int GetDepth() { return depth; }
		bool IsUp() { return index != -1; }
	};

	T* parent;
	vector<T*> childs;
	int index;

	Iterator begin(bool up = false)
	{
		return Iterator(static_cast<T*>(this), up);
	}

	Iterator end()
	{
		return Iterator(nullptr, false);
	}
};

//-----------------------------------------------------------------------------
// Get N best entry/value pairs
template<typename T, uint COUNT, typename ValueT = int>
struct TopN
{
	TopN(T default_best, ValueT default_value)
	{
		for(uint i = 0; i < COUNT; ++i)
		{
			best[i] = default_best;
			best_values[i] = default_value;
		}
	}

	void Add(T entry, ValueT value)
	{
		for(uint i = 0; i < COUNT; ++i)
		{
			if(value > best_values[i])
			{
				for(uint j = COUNT - 1; j > i; --j)
				{
					best_values[j] = best_values[j - 1];
					best[j] = best[j - 1];
				}
				best[i] = entry;
				best_values[i] = value;
				break;
			}
		}
	}

	bool Is(T entry) const
	{
		for(uint i = 0; i < COUNT; ++i)
		{
			if(best[i] == entry)
				return true;
		}
		return false;
	}

	T best[COUNT];
	ValueT best_values[COUNT];
};
