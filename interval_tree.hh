/*
 * rbtree.hh
 *
 *  Created on: Mar 6, 2017
 *      Author: simonm
 */

#ifndef INTERVALTREE_HH_
#define INTERVALTREE_HH_

#include "rbtree.hh"

#include <memory>
#include <exception>
#include <algorithm>


template<typename I,typename V>
class interval_node_t
{
  public:

    template<typename, typename> friend class interval_tree;
    template<typename, typename, typename> friend class rbtree;
    friend class interval_tree_tester;


    public:

      interval_node_t( I low, I high, const V &value ) :
        low( low ), high( high ), value( value ), key( low ), max( high ), colour( RED ), parent( nullptr ) { }

      const I low;
      const I high;
      V value;

    private:
      const I &key;
      I max;
      colour_t colour;
      interval_node_t* parent;

      std::unique_ptr<interval_node_t> left;
      std::unique_ptr<interval_node_t> right;
};

template<typename I, typename V>
class interval_tree : public rbtree< I, V, interval_node_t<I, V> >
{
  private:

    typedef interval_node_t<I, V> N;

    typedef typename rbtree<I, V, N>::leaf_node_t leaf_node_t;

    std::unique_ptr<N> make_node( I low, I high, const V &value )
    {
      return std::unique_ptr<N>( new N( low, high, value ) );
    }

  public:

    typedef typename rbtree<I, V, N>::accessor accessor;

    virtual ~interval_tree()
    {

    }

    void insert( I low, I high, const V &value )
    {
      insert_into( low, high, value, this->tree_root );
    }

    void erase( I low, I high )
    {
      std::unique_ptr<N> &node = this->find_in( low, this->tree_root );
      if( !node || node->low != low || node->high != high )
        return;
      this->erase_node( node );
    }

    std::vector<accessor> query( I point )
    {
      std::vector<accessor> result;
      query( point, this->tree_root, result );
      return result;
    }

  private:

    using rbtree<I, V, N>::insert;
    using rbtree<I, V, N>::erase;
    using rbtree<I, V, N>::find;

    static void query( I point, const std::unique_ptr<N> &node, std::vector<accessor> &result )
    {
      // base case
      if( !node ) return;

      // the point is to the right of the rightmost point of any interval
      if( p > node->max ) return;

      // check this node
      if( node->low <= p && node->high >= p )
        result.push_back( accessor( node.get() ) );

      // if p is to the left of the start of this interval,
      // then it can't be in any child to the right
      if( p < node->low )
      {
        query( point, node->left, result );
        return;
      }

      // the only thing left is to check the right subtree
      query( point, node->right, result );
    }

    void insert_into( I low, I high, const V &value, std::unique_ptr<N> &node, N *parent = nullptr )
    {
      if( !node )
      {
        node = make_node( low, high, value );
        node->parent = parent;
        ++this->tree_size;
        update_max( node->parent, node->max );
        this->rb_insert_case1( node.get() );
        return;
      }

      if( low == node->low )
        return;

      if( low < node->low )
        insert_into( low, high, value, node->left, node.get() );
      else
        insert_into( low, high, value, node->right, node.get() );
    }

    void erase_node( std::unique_ptr<N> &node )
    {
      if( !node ) return;

      if( this->has_two( node.get() ) )
      {
        // in this case:
        // 1. look for the in-order successor
        // 2. replace the node with the in-order successor
        // 3. erase the in-order successor
        std::unique_ptr<N> &successor = this->find_successor( node );
        this->swap_successor( node, successor );
        // we don't update max since in erase_node we
        // will do it after removing respective node
        erase_node( successor );
        return;
      }

      // node has at most one child
      // in this case simply replace the node with the
      // single child or null if there are no children
      N *parent = node->parent;
      std::unique_ptr<N> &child = node->left ? node->left : node->right;
      colour_t old_colour = node->colour;
      if( child )
        child->parent = node->parent;
      node.reset( child.release() );
      update_max( parent );
      --this->tree_size;
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
          this->rb_erase_case1( leaf_node_t( parent ) );
        }
      }
      else if( node )
        // if the node was red it has to have two BLACK children
        // and since at most one of those children is a non-leaf
        // child actually both have to be leafs (null) in order
        // to satisfy the red-black tree invariant
        throw rb_invariant_error();
    }

    void update_max( N *node, I new_high )
    {
      while( node )
      {
        if( node->max < new_high )
        {
          node->max = new_high;
          node = node->parent;
        }
        else
          break;
      }
    }

    void update_max( N *node )
    {
      while( node )
      {
        set_max( node );
        node = node->parent;
      }
    }

    void set_max( N* node )
    {
      if( !node->left && !node->right )
      {
        node->max = node->high;
        return;
      }

      if( !node->left || !node->right )
      {
        node->max = std::max( node->high, ( node->left ? node->left->max : node->right->max ) );
        return;
      }

      node->max = std::max( node->high, std::max( node->left->max, node->right->max ) );
    }

    virtual void right_rotation( N *node )
    {
      N *pivot = node->left.get();
      rbtree<I, V, N>::right_rotation( node );
      set_max( node ); // set first max for node since now it's lower in the tree
      set_max( pivot );
    }

    virtual void left_rotation( N *node )
    {
      N* pivot = node->right.get();
      rbtree<I, V, N>::left_rotation( node );
      set_max( node ); // set first max for node since now it's lower in the tree
      set_max( pivot );
    }
};

#endif /* INTERVALTREE_HH_ */
