#include <bits/stdc++.h>
 
using namespace std;
 
int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int N, K;
    cin >> N >> K;
    
    vector<long long> a(N);
    for (int i = 0; i < N; i++) {
        cin >> a[i];
    }
    
    sort(a.begin(), a.end());
    
    long long result = 0;
    int left = 0;
 
    for (int right = 0; right < N; right++) {
 
        while (a[right] - a[left] > K) {
            left++;
        }
 
        long long count = right - left;
 
        if (count >= 2) {
            result += count * (count - 1) / 2;
        }
    }
    
    cout << result << endl;
    
    return 0;
}
