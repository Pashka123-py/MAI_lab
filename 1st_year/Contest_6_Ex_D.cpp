#include <bits/stdc++.h>
using namespace std;
 
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int n, m, start, finish;
    cin >> n >> m >> start >> finish;
    start--; finish--;
    
    vector<vector<int>> graph(n);
    for (int i = 0; i < m; i++) {
        int u, v;
        cin >> u >> v;
        u--; v--;
        graph[u].push_back(v);
        graph[v].push_back(u);
    }
    
    // Сортируем списки смежности для лексикографического порядка
    for (int i = 0; i < n; i++) {
        sort(graph[i].begin(), graph[i].end());
    }
    
    vector<int> dist(n, -1);
    vector<int> parent(n, -1);
    queue<int> q;
    
    dist[start] = 0;
    q.push(start);
    
    while (!q.empty()) {
        int v = q.front();
        q.pop();
        
        if (v == finish) break;
        
        for (int to : graph[v]) {
            if (dist[to] == -1) {
                dist[to] = dist[v] + 1;
                parent[to] = v;
                q.push(to);
            }
        }
    }
    
    if (dist[finish] == -1) {
        cout << "-1\n";
    } else {
        vector<int> path;
        for (int v = finish; v != -1; v = parent[v]) {
            path.push_back(v);
        }
        reverse(path.begin(), path.end());
        
        for (size_t i = 0; i < path.size(); i++) {
            cout << path[i] + 1;
            if (i + 1 < path.size()) cout << " ";
        }
        cout << "\n";
    }
    
    return 0;
}
