#include <bits/stdc++.h>
using namespace std;
 
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    long long cnt1, cnt2, x, y;
    cin >> cnt1 >> cnt2 >> x >> y;
    
    long long l = 1, r = 2e18, ans = r;
    long long xy = x * y;
    
    while (l <= r) {
        long long v = (l + r) / 2;
        
        long long both = v / xy;
        long long only_x = v / x - both;
        long long only_y = v / y - both;
        long long free = v - only_x - only_y - both;
        
        long long need1 = max(0LL, cnt1 - only_y);
        long long need2 = max(0LL, cnt2 - only_x);
        
        if (need1 + need2 <= free) {
            ans = v;
            r = v - 1;
        } else {
            l = v + 1;
        }
    }
    
    cout << ans << endl;
    
    return 0;
}
