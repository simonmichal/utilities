/*
 * rbtree.hh
 *
 *  Created on: Mar 6, 2017
 *      Author: simonm
 */

#ifndef RBTREE_HH_
#define RBTREE_HH_

#include <memory>
#include <string>
#include <iostream>
#include <exception>

class rb_invariant_error : public std::exception
{
  public:

    rb_invariant_error() {}

    virtual const char* what() const throw()
    {
      return "Red-black tree invariant violation!";
    }

};

template<typename K, typename V>
class rbtree
{
  private:

    enum colour_t
    {
      RED = true,
      BLACK = false
    };

    struct node_t
    {
      node_t( const K &key, const V &value ) : key( key ), value( value ), colour( RED ), parent( nullptr ) { }

      void swap( node_t *node )
      {
        std::swap( key, node->key );
        std::swap( value, node->value );
      }

      K key;
      V value;
      colour_t colour;
      node_t* parent;

      std::unique_ptr<node_t> left;
      std::unique_ptr<node_t> right;
    };

    static std::unique_ptr<node_t> make_node( const K &key, const V &value )
    {
      return std::unique_ptr<node_t>( new node_t( key, value ) );
    }

    static std::unique_ptr<node_t> null_node;

    struct accessor_proxy
    {
      accessor_proxy( node_t *node ) : key( node->key ), value( node->value ) { }
      const K &key;
      V &value;
    };

    // this class is just used in rb_erase_case# methods as
    // they need to accept a leaf (null) node as a parameter
    // that can return its parent
    struct leaf_node_t
    {
        leaf_node_t( node_t *parent ) : colour( BLACK ), parent( parent ) { }

        leaf_node_t( const leaf_node_t &leaf ) : colour( leaf.colour ), parent( leaf.parent ) { }

        leaf_node_t& operator=( const leaf_node_t &leaf )
        {
          colour = leaf.colour;
          parent = leaf.parent;
          return *this;
        }

        leaf_node_t* operator->()
        {
          return this;
        }

        leaf_node_t& operator*()
        {
          return *this;
        }

        operator bool() const
        {
          return true;
        }

        bool operator==( node_t *node ) const
        {
          return node == nullptr;
        }

        colour_t colour;
        node_t *parent;
    };

  public:

    void print()
    {
      print( tree_root );
    }

    void print( const std::unique_ptr<node_t> &root, const std::string &indent = "" )
    {
      if( !root ) return;

      print( root->right, indent + "  " );
      std::string colour = root->colour ? "(R)" : "(B)";
      std::cout << indent << root->key << colour << std::endl;
      print( root->left, indent + "  " );
    }

    class accessor
    {
      public:

        accessor( node_t *node ) : proxy( node ? new accessor_proxy( node ) : nullptr ) { }

        accessor_proxy* operator->()
        {
          return proxy.get();
        }

        accessor_proxy& operator*()
        {
          return *proxy.get();
        }

        operator bool() const
        {
          return bool( proxy );
        }

      private:

        std::unique_ptr<accessor_proxy> proxy;
    };

    rbtree() : tree_size( 0 ) { }

    virtual ~rbtree() { }

    void insert( const K &key, const V &value )
    {
      insert_into( key, value, tree_root );
    }

    void erase( const K &key )
    {
      std::unique_ptr<node_t> &node = find_in( key, tree_root );
      erase_node( node );
    }

    void clear()
    {
      tree_root.reset();
    }

    accessor find( const K &key )
    {
      const std::unique_ptr<node_t> &n = find_in( key, tree_root );
      return accessor( n.get() );
    }

    const accessor find( const K &key ) const
    {
      const std::unique_ptr<node_t> &n = find_in( key, tree_root );
      return accessor( n.get() );
    }

    size_t size() const
    {
      return tree_size;
    }

    bool empty() const
    {
      return !tree_root;
    }

    bool test_invariant()
    {
      return test_tree_invariant( tree_root ).first;
    }

  private:

    static std::pair<bool, int> test_tree_invariant( const std::unique_ptr<node_t> &root )
    {
      // base case
      if( !root )
        return std::make_pair( true, 0 );

      int black = 0;
      if( root->colour == RED )
      {
        // RED node cannot have RED children
        if( ( root->left && root->left->colour == RED ) || ( root->right && root->right->colour == RED ) )
          return std::make_pair( false, -1 );
      }
      else
        black += 1;

      std::pair<bool, int> l = test_tree_invariant( root->left );
      std::pair<bool, int> r = test_tree_invariant( root->right );

      if( !l.first || !r.first )
        return std::make_pair( false, -1 );

      if( l.second != r.second )
        return std::make_pair( false, -1 );

      return std::make_pair( true, l.second + black );
    }

    void insert_into( const K &key, const V &value, std::unique_ptr<node_t> &node, node_t *parent = nullptr )
    {
      if( !node )
      {
        node = make_node( key, value );
        node->parent = parent;
        ++tree_size;
        rb_insert_case1( node.get() );
        return;
      }

      if( key == node->key )
        return;

      if( key < node->key )
        insert_into( key, value, node->left, node.get() );
      else
        insert_into( key, value, node->right, node.get() );
    }

    void erase_node( std::unique_ptr<node_t> &node )
    {
      if( !node ) return;

      if( has_two( node.get() ) )
      {
        // in this case:
        // 1. look for the in-order successor
        // 2. replace the node with the in-order successor
        // 3. erase the in-order successor
        std::unique_ptr<node_t> &successor = find_successor( node );
        node->swap( successor.get() );
        erase_node( successor );
        return;
      }

      // node has at most one child
      // in this case simply replace the node with the
      // single child or null if there are no children
      node_t *parent = node->parent;
      std::unique_ptr<node_t> &child = node->left ? node->left : node->right;
      colour_t old_colour = node->colour;
      if( child ) child->parent = node->parent;
      node.reset( child.release() );
      --tree_size;
      if( old_colour == BLACK)
      {
        if( node && node->colour == RED )
          node->colour = BLACK;
        else
        {
          // if we are here the node is null because a BLACK
          // node that has at most one non-leaf child must
          // have two null children (null children are BLACK)
          if( node ) throw rb_invariant_error();
          rb_erase_case1( leaf_node_t( parent ) );
        }
      }
      else if( node )
        // if the node was red it has to have two BLACK children
        // and since at most one of those children is a non-leaf
        // child actually both have to be leafs (null) in order
        // to satisfy the red-black tree invariant
        throw rb_invariant_error();

    }

    template<typename PTR> // make it a template so it works both for constant and mutable pointers
    static PTR& find_in( const K &key, PTR &node )
    {
      if( !node ) return null_node;

      if( key == node->key )
        return node;

      if( key < node->key )
        return find_in( key, node->left );
      else
        return find_in( key, node->right );
    }

    template<typename PTR> // make it a template so it works both for constant and mutable pointers
    static PTR& find_min( PTR &node )
    {
      if( !node ) return null_node;
      if( node->left )
        return find_min( node->left );
      return node;
    }

    template<typename PTR> // make it a template so it works both for constant and mutable pointers
    static PTR& find_successor( PTR &node )
    {
      if( !node ) return null_node;
      return find_min( node->right );
    }

    static bool has_two( const node_t *node )
    {
      return node->left && node->right;
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static void replace( std::unique_ptr<node_t> &ptr, node_t *node )
    {
      ptr.release();
      ptr.reset( node );
    }

    void right_rotation( node_t *node )
    {
      if( !node ) return;

      node_t *parent = node->parent;
      node_t *left_child = node->left.release();

      bool is_left = ( parent && parent->left.get() == node ) ? true : false;

      node->left.reset( left_child->right.release() );
      if( node->left ) node->left->parent = node;

      left_child->right.reset( node );
      if( left_child->right ) left_child->right->parent = left_child;

      left_child->parent = parent;
      if( !parent )
        replace( tree_root, left_child );
      else if( is_left )
        replace( parent->left, left_child );
      else
        replace( parent->right, left_child );
    }

    void left_rotation( node_t *node )
    {
      if( !node ) return;

      node_t *parent = node->parent;
      node_t *right_child = node->right.release();

      bool is_left = ( parent && parent->left.get() == node ) ? true : false;

      node->right.reset( right_child->left.release() );
      if( node->right ) node->right->parent = node;

      right_child->left.reset( node );
      if( right_child->left ) right_child->left->parent = right_child;

      right_child->parent = parent;
      if( !parent )
        replace( tree_root, right_child );
      else if( is_left )
        replace( parent->left, right_child );
      else
        replace( parent->right, right_child );
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

    node_t* get_grandparent( node_t *node )
    {
      if( !node || !node->parent ) return nullptr;
      return node->parent->parent;
    }

    node_t* get_uncle( node_t *node )
    {
      node_t *grandparent = get_grandparent( node );
      if( !grandparent ) return nullptr;
      if( grandparent->left.get() == node->parent )
        return grandparent->right.get();
      else
        return grandparent->left.get();
    }

    void rb_insert_case1( node_t *node )
    {
      if( node->parent == nullptr ) // it is the root
        node->colour = BLACK;
      else
        rb_insert_case2( node );
    }

    void rb_insert_case2( node_t *node )
    {
      if( node->parent->colour == BLACK )
        return; // the invariant is OK
      else
        rb_insert_case3( node );
    }

    void rb_insert_case3( node_t *node )
    {
      node_t *uncle = get_uncle( node );
      if( uncle && uncle->colour == RED )
      {
        node->parent->colour = BLACK;
        uncle->colour = BLACK;
        node_t *grandparent = get_grandparent( node );
        grandparent->colour = RED;
        rb_insert_case1( grandparent );
      }
      else
        rb_insert_case4( node );
    }

    void rb_insert_case4( node_t *node )
    {
      node_t *grandparent = get_grandparent( node );

      if( ( node == node->parent->right.get() ) && ( node->parent == grandparent->left.get() ) )
      {
        left_rotation( grandparent->left.get() );
        node = node->left.get();
      }
      else if( ( node == node->parent->left.get() ) && ( node->parent == grandparent->right.get() ) )
      {
        right_rotation( grandparent->right.get() );
        node = node->right.get();
      }

      rb_insert_case5( node );
    }

    void rb_insert_case5( node_t *node )
    {
      node_t *grandparent = get_grandparent( node );
      node->parent->colour = BLACK;
      grandparent->colour = RED;
      if( node == node->parent->left.get() )
        right_rotation( grandparent );
      else
        left_rotation( grandparent );
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename NODE>
    static bool is_left( NODE node )
    {
      return node == node->parent->left.get();
    }

    template<typename NODE>
    static bool is_right( NODE node )
    {
      return node == node->parent->right.get();
    }

    template<typename NODE>
    node_t* get_sibling( NODE node )
    {
      if( !node && !node->parent )
        return nullptr;
      if( is_left( node ) )
        return node->parent->right.get();
      else
        return node->parent->left.get();
    }

    template<typename NODE>
    void rb_erase_case1( NODE node )
    {
      if( node->parent != nullptr )
        rb_erase_case2( node );
    }

    template<typename NODE>
    void rb_erase_case2( NODE node )
    {
      node_t *sibling = get_sibling( node );
      if( sibling->colour == RED )
      {
        node->parent->colour = RED;
        sibling->colour = BLACK;
        if( is_left( node ) )
          left_rotation( node->parent );
        else
          right_rotation( node->parent );
      }

      rb_erase_case3( node );
    }

    template<typename NODE>
    void rb_erase_case3( NODE node )
    {
      node_t *sibling = get_sibling( node );
      colour_t sibling_left_colour = sibling->left ? sibling->left->colour : BLACK;
      colour_t sibling_right_colour = sibling->right ? sibling->right->colour : BLACK;
      if( node->parent->colour == BLACK &&
          sibling->colour == BLACK &&
          sibling_left_colour == BLACK &&
          sibling_right_colour == BLACK )
      {
        sibling->colour = RED;
        rb_erase_case1( node->parent );
      }
      else
        rb_erase_case4( node );
    }

    template<typename NODE>
    void rb_erase_case4( NODE node )
    {
      node_t *sibling = get_sibling( node );
      colour_t sibling_left_colour = sibling->left ? sibling->left->colour : BLACK;
      colour_t sibling_right_colour = sibling->right ? sibling->right->colour : BLACK;
      if( node->parent->colour == RED &&
          sibling->colour == BLACK &&
          sibling_left_colour == BLACK &&
          sibling_right_colour == BLACK )
      {
        sibling->colour = RED;
        node->parent->colour = BLACK;
      }
      else
        rb_erase_case5( node );
    }

    template<typename NODE>
    void rb_erase_case5( NODE node )
    {
      node_t *sibling = get_sibling( node );
      colour_t sibling_left_colour = sibling->left ? sibling->left->colour : BLACK;
      colour_t sibling_right_colour = sibling->right ? sibling->right->colour : BLACK;
      if( sibling->colour == BLACK )
      {
        if( is_left( node ) &&
            sibling_right_colour == BLACK &&
            sibling_left_colour == RED )
        {
          sibling->colour = RED;
          if( sibling->left ) sibling->left->colour = BLACK;
          right_rotation( sibling );
        }
        else if( is_right( node ) &&
                 sibling_left_colour == BLACK &&
                 sibling_right_colour == RED )
        {
          sibling->colour = RED;
          if( sibling->right ) sibling->right->colour = BLACK;
          left_rotation( sibling );
        }
      }

      rb_erase_case6( node );
    }

    template<typename NODE>
    void rb_erase_case6( NODE node )
    {
      node_t *sibling = get_sibling( node );
      sibling->colour = node->parent->colour;
      node->parent->colour = BLACK;
      if( is_left( node ) )
      {
        if( sibling->right ) sibling->right->colour = BLACK;
        left_rotation( node->parent );
      }
      else
      {
        if( sibling->left ) sibling->left->colour = BLACK;
        right_rotation( node->parent );
      }
    }

    std::unique_ptr<node_t> tree_root;
    size_t tree_size;
};

template<typename K, typename V>
std::unique_ptr< typename rbtree<K, V>::node_t > rbtree<K, V>::null_node;

#endif /* RBTREE_HH_ */