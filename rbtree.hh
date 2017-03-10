/*
 * rbtree.hh
 *
 *  Created on: Mar 6, 2017
 *      Author: simonm
 */

#ifndef RBTREE_HH_
#define RBTREE_HH_

#include <memory>
#include <exception>
#include <iostream>

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
    friend class rbtree_tester;

  private:

    enum colour_t
    {
      RED = true,
      BLACK = false
    };

    class node_t
    {
      friend class rbtree;
      friend class rbtree_tester;

      public:
        node_t( const K &key, const V &value ) : key( key ), value( value ), colour( RED ), parent( nullptr ) { }

        const K key;
        V value;

        void print()
        {
          std::cout << std::endl;
          std::cout << "key: " << key << std::endl;
          if( parent ) std::cout << "parent key: " << parent->key << std::endl;
          else std::cout << "I'm root" << std::endl;
          if( left ) std::cout << "left key: " << left->key << std::endl;
          else std::cout << "Left is a leaf" << std::endl;
          if( right ) std::cout << "right key: " << right->key << std::endl;
          else std::cout << "Right is a leaf" << std::endl;
        }

      private:
        colour_t colour;
        node_t* parent;

        std::unique_ptr<node_t> left;
        std::unique_ptr<node_t> right;
    };

    static std::unique_ptr<node_t> make_node( const K &key, const V &value )
    {
      return std::unique_ptr<node_t>( new node_t( key, value ) );
    }

    static void swap_right_child( std::unique_ptr<node_t> &node, std::unique_ptr<node_t> &successor )
    {
      std::swap( node->colour, successor->colour );
      // first do the obvious
      std::swap( node->left, successor->left );
      if( node->left ) node->left->parent = node.get();
      if( successor->left ) successor->left->parent = successor.get();
      // now gather remaining pointers
      node_t *p = node->parent;
      node_t *n = node.release();
      node_t *s = successor.release();
      node_t *s_right = s->right.release();
      // and finally reassign those pointers
      s->parent = p;
      node.reset( s );
      s->right.reset( n );
      n->parent = s;
      n->right.reset( s_right );
      if( s_right ) s_right->parent = n;
    }

    static void swap_successor( std::unique_ptr<node_t> &node, std::unique_ptr<node_t> &successor )
    {
      // first check if successor is a direct child of node,
      // since it is the in-order successor it can be only
      // the right child

      if( node->right.get() == successor.get() )
      {
        // it is the right child
        swap_right_child( node, successor );
        return;
      }
      // swap colour
      std::swap( node->colour, successor->colour );
      // swap parents
      std::swap( node, successor );
      std::swap( node->parent, successor->parent );
      // swap left
      std::swap( node->left, successor->left );
      if( node->left ) node->left->parent = node.get();
      if( successor->left ) successor->left->parent = successor.get();
      // swap right
      std::swap( node->right, successor->right );
      if( node->right ) node->right->parent = node.get();
      if( successor->right ) successor->right->parent = successor.get();
    }

    static std::unique_ptr<node_t> null_node;

    // this class is just used in rb_erase_case# methods as
    // they need to accept a leaf (null) node as an argument
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
        node_t  *parent;
    };

  public:

    class accessor
    {
      public:

        accessor( node_t *node ) : node( node ) { }

        node_t* operator->()
        {
          return node;
        }

        node_t& operator*()
        {
          return *node;
        }

        operator bool() const
        {
          return bool( node );
        }

      private:

        node_t *node;
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

   private:

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
        swap_successor( node, successor );
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
      if( !sibling ) throw rb_invariant_error();
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
      if( !sibling ) throw rb_invariant_error();
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
      if( !sibling ) throw rb_invariant_error();
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
      if( !sibling ) throw rb_invariant_error();
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
      if( !sibling ) throw rb_invariant_error();
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