// Created by 93738 on 2019/6/2.
//

#ifndef BPT_BTREEE_HPP
#define BPT_BTREEE_HPP

# include <fstream>
# include <functional>
# include <cstdio>
# include <cstddef>
# include <cstring>
# include <utility>
# include <search.h>
# include "exception.hpp"
# include"utility.hpp"

using std::pair;
using std::make_pair;

namespace sjtu {
    template<class Key, class Value, class Compare = std::less<Key> >
    class BTree {
    public:
        typedef std::pair<Key, Value> value_type;

        class iterator;

        class const_iterator;

    private:

        static const int M = 226;
        static const int L = 30;


        struct information {
            int head_leaf;
            int tail_leaf;
            int root_tree;
            size_t size_tree;
            int eof;

            information() : head_leaf(0), tail_leaf(0), root_tree(0), eof(0), size_tree(0) {}
        };


        struct normalnode {
            long offset;
            int parent;
            int child[M + 1];
            Key key[M];
            int cur;
            bool child_isleaf = false;

            normalnode() {
                offset = 0;
                parent = 0;
                cur = 0;
                memset(child, 0, sizeof(child));
                memset(key, 0, sizeof(key));
            }
        };

        struct leafnode {
            long offset;
            long parent;
            int cur;
            long pre, next;
            value_type val[L + 1];

            leafnode() {
                offset = 0;
                parent = 0;
                cur = 0;
                pre = next = 0;
                for(int i=0;i<L+1;++i){
                    val[i].first=0;
                    val[i].second=0;
                }
            }
        };


        information info;
        long info_offset = 0;
        FILE *f1;
        char f1_name[20] ="xixixi.txt" ;

        bool f1_isopen = false;
        bool f1_exists =false;

        inline void
        readfile(void *read_place, long offset, size_t num, size_t size)//put the information into a pointer;
        {
            //if (fseek(f1, offset, SEEK_SET)) ;//throw "openf failed";

            fseek(f1, offset, SEEK_SET);
            fread(read_place, num, size, f1);
        }

        inline void openfile() {
            f1_exists = 1;
            if (f1_isopen == 0) {
                f1 = fopen(f1_name, "rb+");
                //if hasn't been open,open it but rb+ can only open
                //can't create.
                if (f1 == nullptr){//open shibaire.
                    f1_exists = 0;
                    f1 = fopen(f1_name, "w");//w create a file;
                    fclose(f1);
                    f1 = fopen(f1_name, "rb+");//using a proper way reopen;
                } else readfile(&info, info_offset, 1, sizeof(info));
                f1_isopen = 1;
            }
        }


        inline void closefile() {
            if (f1_isopen)
                fclose(f1);
            f1_isopen = 0;
        }

        inline void writefile(void *write_place, long offset, size_t num, size_t size) {
            //if (fseek(f1, offset, SEEK_SET)) {throw "openfile failed"; }
            fseek(f1, offset, SEEK_SET);
            fwrite(write_place, num, size, f1);
        }


        void build_tree() {
            info.size_tree = 0;
            info.eof = sizeof(information);
            normalnode root;
            leafnode leaf1;
            info.root_tree = root.offset = info.eof;
            info.eof += sizeof(root);
            info.head_leaf = info.tail_leaf = leaf1.offset = info.eof;
            info.eof += sizeof(leaf1);
            root.child[0] = leaf1.offset;
            root.cur = 1;
            root.child_isleaf = 1;
            leaf1.parent = root.offset;
            writefile(&info, info_offset, 1, sizeof(info));
            writefile(&root, root.offset, 1, sizeof(root));
            writefile(&leaf1, leaf1.offset, 1, sizeof(leaf1));

        }

        //return the offset of the leaf,give the key and the offset for loop;
        long find_leaf(const Key &key, long offset) {
            normalnode tmp;
            readfile(&tmp, offset, 1, sizeof(normalnode));
            if (tmp.child_isleaf) {
                int i = 0;
                for (i = 0; i < tmp.cur - 1; ++i)//the number of key is child -1
                    if (key < tmp.key[i]) break;
                //if (i == tmp.cur - 1)
                //  return 0;
                return tmp.child[i];
            } else {
                int i = 0;
                for (i = 0; i < tmp.cur - 1; ++i)
                    if (key < tmp.key[i]) break;
                //if (i == tmp.cur - 1)
                //  return 0;
                long off = tmp.child[i];
                return find_leaf(key, off);
            }

        }

        std::pair<iterator, OperationResult> insert_toleaf(leafnode &leaf, const value_type &v) {
            std::pair<iterator, OperationResult> p;

            int pos = 0;
            for (; pos < leaf.cur; ++pos) {
                if (v.first == leaf.val[pos].first) {

                    return std::pair<iterator, OperationResult>(iterator(), Fail);
                }
                if (v.first < leaf.val[pos].first) break;
            }
            leaf.cur++;
            info.size_tree++;
            for (int j = leaf.cur - 1; j > pos; --j) {
                leaf.val[j].first = leaf.val[j - 1].first;
                leaf.val[j].second = leaf.val[j - 1].second;
            }

            leaf.val[pos].first = v.first;
            leaf.val[pos].second = v.second;

            p.first.from = this;
            p.first.pos = pos;
            p.first.offset = leaf.offset;
            writefile(&info, info_offset, 1, sizeof(info));
            if (leaf.cur <= L) writefile(&leaf, leaf.offset, 1, sizeof(leaf));
            else spilt_leaf(leaf, p.first, v.first);
            p.second = Success;
            return p;


        }

        //insert a newnode(both leaf and normal) to its parent,if cannot,spilt parent;
        //only have an operation on the parent not child;
        void insert_tonode(normalnode &node, Key &newkey, long offset) {
            int pos = 0;
            for (; pos < node.cur - 1; ++pos)
                if (newkey < node.key[pos]&&newkey==node.key[pos]) break;
            for (int i = node.cur; i > pos + 1; --i)
                node.child[i] = node.child[i - 1];
            for (int i = node.cur - 1; i > pos; --i)
                node.key[i] = node.key[i - 1];
            node.key[pos] = newkey;
            node.child[pos + 1] = offset;
            ++node.cur;
            if (node.cur <= M) writefile(&node, node.offset, 1, sizeof(node));
            else spilt_node(node);
        }

        //spilt the it and the key is uesd to place the changed iterator
        void spilt_leaf(leafnode &leaf, iterator &it, Key key) {
            leafnode leafnew;
            normalnode parent;

            readfile(&parent, leaf.parent, 1, sizeof(normalnode));
            leafnew.offset = info.eof;
            info.eof += sizeof(leafnew);
            if (info.tail_leaf == leaf.offset)
                info.tail_leaf = leafnew.offset;
            //if (parent.cur < M - 1) {//insert directly
            leaf.cur = L / 2;
            leafnew.cur = L / 2 + 1;
            leafnew.pre = leaf.offset;
            leafnew.parent = parent.offset;
            leafnew.next = leaf.next;
            leaf.next = leafnew.offset;
            for (int i = 0; i < L / 2 + 1; ++i) {
                leafnew.val[i] = leaf.val[i + L / 2];

                if (leafnew.val[i].first == key) {
                    it.offset = leafnew.offset;
                    it.pos = i;
                }
            }
            leafnode nxt;
            if (leafnew.next == 0);
            else {
                readfile(&nxt, leafnew.next, 1, sizeof(nxt));
                nxt.pre = leafnew.offset;
                writefile(&nxt, nxt.offset, 1, sizeof(nxt));
            }

            writefile(&leaf, leaf.offset, 1, sizeof(leafnode));
            writefile(&leafnew, leafnew.offset, 1, sizeof(leafnew));
            writefile(&info, info_offset, 1, sizeof(info));

            //insert to the parent ,in that function,in sequence;
            normalnode par;
            readfile(&par, leaf.parent, 1, sizeof(par));
            insert_tonode(par, leafnew.val[0].first, leafnew.offset);

        }

        void spilt_node(normalnode &node) {
            normalnode newnode;
            newnode.offset = info.eof;
            info.eof += sizeof(newnode);
            node.cur = node.cur / 2;
            newnode.cur = M / 2 + 1;
            newnode.parent = node.parent;
            newnode.child_isleaf = node.child_isleaf;
            /*int pos = 0;
            for (; pos <= M - 1; ++pos)
                if (node.key[M - 1] <= node.key[pos]) break;
            Key tmpk = node.key[M - 1];
            long tmpc = node.child[M];
            for (int j = M - 1; j > pos; --j)
                node.key[j] = node.key[j - 1];
            for (int j = M; j > pos + 1; --j)
                node.child[j] = node.child[j - 1];
            node.key[pos] = tmpk;
            node.child[pos + 1] = tmpc;*/
            //useless operation for sort;
            for (int i = 0; i < newnode.cur - 1; ++i)
                newnode.key[i] = node.key[i + node.cur - 1 + 1];//新节点的第一个key自动和第二个合并，避免出现空数据块。
            for (int i = 0; i < newnode.cur; ++i)
                newnode.child[i] = node.child[i + node.cur];

            leafnode leaf;
            normalnode normal;
            for (int i = 0; i < newnode.cur; ++i) {
                if (newnode.child_isleaf == 1) {
                    readfile(&leaf, newnode.child[i], 1, sizeof(leaf));
                    leaf.parent = newnode.offset;
                    writefile(&leaf, leaf.offset, 1, sizeof(leafnode));
                } else {
                    readfile(&normal, newnode.child[i], 1, sizeof(normal));
                    normal.parent = newnode.offset;
                    writefile(&normal, normal.offset, 1, sizeof(normal));
                }
            }

            if (node.offset == info.root_tree) {
                normalnode newroot;
                newroot.parent = 0;
                newroot.child_isleaf = 0;
                newroot.offset = info.eof;
                info.eof += sizeof(newroot);
                newroot.cur = 2;
                newroot.key[0] = newnode.key[0];
                newroot.child[0] = node.offset;
                newroot.child[1] = newnode.offset;
                node.parent = newroot.offset;
                newnode.parent = newroot.offset;
                info.root_tree = newroot.offset;

                writefile(&info, info_offset, 1, sizeof(info));
                writefile(&node, node.offset, 1, sizeof(node));
                writefile(&newnode, newnode.offset, 1, sizeof(newnode));
                writefile(&newroot, newroot.offset, 1, sizeof(newroot));
            } else {
                writefile(&info, info_offset, 1, sizeof(info));
                writefile(&node, node.offset, 1, sizeof(node));
                writefile(&newnode, newnode.offset, 1, sizeof(newnode));

                normalnode parent;
                readfile(&parent, node.parent, 1, sizeof(parent));
                insert_tonode(parent, newnode.key[0], newnode.offset);
            }

        }


    public:


        class iterator {
            friend class BTree;

            friend class const_iterator;

        private:
            long offset;// relate to the leafnode;
            int pos; //relate to the pos in the leafnode;
            BTree *from;
        public:
            bool modify(const Value &value) {
                return  true;

            }

            iterator() {

                offset = 0;
                pos = 0;
                from = nullptr;
                // TODO Default Constructor
            }

            iterator(const iterator &other) {
                offset = other.offset;
                pos = other.pos;
                from = other.from;
                // TODO Copy Constructor
            }

            // Return a new iterator which points to the n-next elements
            iterator operator++(int) {
                iterator it(*this);
                if (*this == from->end()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return it;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos < leaf.cur - 1) {
                    pos++;
                    return it;
                } else {
                    pos = 0;
                    offset = leaf.next;
                    return it;
                }



                // Todo iterator++
            }

            iterator &operator++() {
                if (*this == from->end()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return *this;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos < leaf.cur - 1) {
                    pos++;
                    return *this;
                } else {
                    pos = 0;
                    offset = leaf.next;
                    return *this;
                }
                // Todo ++iterator
            }

            iterator operator--(int) {
                iterator it(*this);
                if (*this == from->begin()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return it;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos > 0) {
                    pos--;
                    return it;
                } else {
                    leafnode pre;
                    from->readfile(&pre, leaf.pre, 1, sizeof(leafnode));
                    pos = pre.cur;
                    offset = leaf.pre;
                    return it;
                }
                // Todo iterator--
            }

            iterator &operator--() {
                if (*this == from->begin()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return *this;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos > 0) {
                    pos--;
                    return *this;
                } else {
                    leafnode pre;
                    from->readfile(&pre, leaf.pre, 1, sizeof(leafnode));
                    pos = pre.cur;
                    offset = leaf.pre;
                    return *this;
                }

                // Todo --iterator
            }


            Value getvalue() {
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                return leaf.val[pos].second;
            }

            Value changevalue(const Value &v) {
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                leaf.val[pos].second=v;
                from->writefile(&leaf,offset,1, sizeof(leaf));
                return v;

            }


            // Overloaded of operator '==' and '!='
            // Check whether the iterators are same
            bool operator==(const iterator &rhs) const {
                return from == rhs.from && pos == rhs.pos && offset == rhs.offset;

            }

            bool operator==(const const_iterator &rhs) const {
                return from == rhs.from && pos == rhs.pos && offset == rhs.offset;
                // Todo operator ==
            }

            bool operator!=(const iterator &rhs) const {
                return (!(from == rhs.from && pos == rhs.pos && offset == rhs.offset));
                // Todo operator !=
            }

            bool operator!=(const const_iterator &rhs) const {
                return (!(from == rhs.from && pos == rhs.pos && offset == rhs.offset));
                // Todo operator !=
            }
        };

        class const_iterator {

            friend class BTree;

            friend class iterator;

        private:
            long offset;// relate to the leafnode;
            int pos; //relate to the pos in the leafnode;
            BTree *from;

        public:
            const_iterator() {
                offset = 0;
                pos = 0;
                from = nullptr;
                // TODO
            }

            const_iterator(const const_iterator &other) {
                offset = other.offset;
                pos = other.pos;
                from = other.from;
                // TODO
            }

            const_iterator(const iterator &other) {
                offset = other.offset;
                pos = other.pos;
                from = other.from;
                // TODO
            }

            const_iterator operator++(int) {
                const_iterator it(*this);
                if (*this == from->end()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return it;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos < leaf.cur - 1) {
                    pos++;
                    return it;
                } else {
                    pos = 0;
                    offset = leaf.next;
                    return it;
                }



                // Todo iterator++
            }

            const_iterator &operator++() {
                if (*this == from->end()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return *this;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos < leaf.cur - 1) {
                    pos++;
                    return *this;
                } else {
                    pos = 0;
                    offset = leaf.next;
                    return *this;
                }

            }

            const_iterator operator--(int) {
                const_iterator it(*this);
                if (*this == from->begin()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return it;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos > 0) {
                    pos--;
                    return it;
                } else {
                    leafnode pre;
                    from->readfile(&pre, leaf.pre, 1, sizeof(leafnode));
                    pos = pre.cur;
                    offset = leaf.pre;
                    return it;
                }
                // Todo iterator--
            }

            const_iterator &operator--() {
                if (*this == from->begin()) {
                    offset = 0;
                    pos = 0;
                    from = nullptr;
                    return *this;
                }
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                if (pos > 0) {
                    pos--;
                    return *this;
                } else {
                    leafnode pre;
                    from->readfile(&pre, leaf.pre, 1, sizeof(leafnode));
                    pos = pre.cur;
                    offset = leaf.pre;
                    return *this;
                }

                // Todo --iterator
            }

            Value getvalue() {
                leafnode leaf;
                from->readfile(&leaf, offset, 1, sizeof(leaf));
                return leaf.val[pos].second;
            }

            // Overloaded of operator '==' and '!='
            // Check whether the iterators are same
            bool operator==(const iterator &rhs) const {
                return from == rhs.from && pos == rhs.pos && offset == rhs.offset;

            }

            bool operator==(const const_iterator &rhs) const {
                return from == rhs.from && pos == rhs.pos && offset == rhs.offset;
                // Todo operator ==
            }

            bool operator!=(const iterator &rhs) const {
                return (!(from == rhs.from && pos == rhs.pos && offset == rhs.offset));
                // Todo operator !=
            }

            bool operator!=(const const_iterator &rhs) const {
                return (!(from == rhs.from && pos == rhs.pos && offset == rhs.offset));
                // Todo operator !=
            }


        };

        // Default Constructor and Copy Constructor
        BTree() {
            f1 = nullptr;
            openfile();
            build_tree();
            // Todo Default
        }

        BTree(const BTree &other) {
            // Todo Copy
        }

        BTree &operator=(const BTree &other) {
            // Todo Assignment
        }

        ~BTree() {
            closefile();
            // Todo Destructor
        }

        // Insert: Insert certain Key-Value into the database
        // Return a pair, the first of the pair is the iterator point to the new
        // element, the second of the pair is Success if it is successfully inserted
        std::pair<iterator, OperationResult> insert(const Key &key, const Value &value) {

            long offset = find_leaf(key, info.root_tree);
            leafnode leaf;
            readfile(&leaf, offset, 1, sizeof(leaf));
            value_type v(key, value);
            std::pair<iterator, OperationResult> it = insert_toleaf(leaf, v);
            return it;

        }

        // Erase: Erase the Key-Value
        // Return Success if it is successfully erased
        // Return Fail if the key doesn't exist in the database
        OperationResult erase(const Key &key) {
            // TODO erase function
            return Fail;  // If you can't finish erase part, just remaining here.
        }

        // Return a iterator to the beginning
        iterator begin() {
            return iterator(this, info.head_leaf, 0);
        }

        const_iterator cbegin() const {
            return const_iterator(this, info.head_leaf, 0);
        }

        // Return a iterator to the end(the next element after the last)
        iterator end() {
            leafnode leaf;
            readfile(&leaf, info.tail_leaf, 1, sizeof(leaf));
            int pos = leaf.cur - 1;
            return iterator(this, info.tail_leaf, pos);

        }

        const_iterator cend() const {
            leafnode leaf;
            readfile(&leaf, info.tail_leaf, 1, sizeof(leaf));
            int pos = leaf.cur - 1;
            return const_iterator(this, info.tail_leaf, pos);

        }

        // Check whether this BTree is empty
        bool empty() const {
            if (info.size_tree == 0) return false;
            else return true;
        }


        // Return the number of <K,V> pairs
        size_t size() const { return info.size_tree; }

        // Clear the BTree
        void clear() {
            f1 = fopen(f1_name, "w");
            fclose(f1);
            openfile();
            build_tree();
        }

        // Return the value refer to the Key(key)
        Value at(const Key &key) {
            long offset;
            offset = find_leaf(key, info.root_tree);
            leafnode leaf;
            readfile(&leaf, offset, 1, sizeof(leaf));
            int pos = 0;
            for (; pos < leaf.cur; ++pos)
                if (key == leaf.val[pos].first) break;
            //if (pos == leaf.cur) //throw " ma wan yi mei you";
            //else
            return leaf.val[pos].second;
        }

        /**
         * Returns the number of elements with key
         *   that compares equivalent to the specified argument,
         * The default method of check the equivalence is !(a < b || b > a)
         */
        size_t count(const Key &key) const {


        }

        /**
         * Finds an element with key equivalent to key.
         * key value of the element to search for.
         * Iterator to an element with key equivalent to key.
         *   If no such element is found, past-the-end (see end()) iterator is
         * returned.
         */
        iterator find(const Key &key) {
            long offset;
            offset = find_leaf(key, info.root_tree);
            leafnode leaf;
            readfile(&leaf, offset, 1, sizeof(leaf));
            int i = 0;
            for (; i < leaf.cur; ++i) {
                if (leaf.val[i].first == key) break;
            }
            if (i == leaf.cur) return end();
            else return iterator(this, offset, i);
        }

        const_iterator find(const Key &key) const {
            long offset;
            offset = find_leaf(key, info.root_tree);
            leafnode leaf;
            readfile(&leaf, offset, 1, sizeof(leaf));
            int i = 0;
            for (; i < leaf.cur; ++i) {
                if (leaf.val[i].first == key) break;
            }
            if (i == leaf.cur) return cend();
            else return const_iterator(this, offset, i);
        }
    };
}  // namespace sjtu


#endif //BPT_BTREEE_HPP
