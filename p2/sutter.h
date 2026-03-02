

template<class Entry>  
std::shared_ptr<Entry> make_entry(int id){ 
    using namespace std;     
	static mutex mutexForCache; // make it thread-safe //    	 
	lock_guard<mutex> hold( mutexForCache );     
	static map<int, weak_ptr<Entry> > cache; // implicit cache, does not own the entries only has weak pointers towards the content //     
	auto sp = cache[id].lock();     
	if(!sp){ 
        sp = make_shared<Entry>(id);         
		cache[id] = sp;     
	} 
    return sp; 
} 